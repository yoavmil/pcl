#include <pcl/test/gtest.h>
#include <pcl/point_types.h>
#include <pcl/point_cloud.h>

#include <pcl/segmentation/extract_polygonal_prism_data.h>

#include <random>

using namespace pcl;
using std::vector;

TEST(ExtractPolygonalPrism, two_rings)
{
  float rMin = 0.1, rMax = 0.25f;
  float dx = 0.5f; // shift the rings from [0,0,0] to [+/-dx, 0, 0]

  // prepare 2 rings
  PointCloud<PointXYZ>::Ptr ring(new PointCloud<PointXYZ>);
  { // use random
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> radiusDist(rMin, rMax);
    std::uniform_real_distribution<float> radianDist(-M_PI, M_PI);
    std::uniform_real_distribution<float> zDist(-0.01f, 0.01f);
    for (size_t i = 0; i < 1000; i++) {
      float radius = radiusDist(gen);
      float angle = radianDist(gen);
      float z = zDist(gen);
      PointXYZ point(std::cosf(angle) * radius, std::sinf(angle) * radius, z);
      ring->push_back(point);
    }
  }

  PointCloud<PointXYZ>::Ptr cloud(new PointCloud<PointXYZ>);
  cloud->reserve(ring->size() * 2);
  for (auto& point : ring->points) {
    auto left = point;
    auto rght = point;
    left.x -= dx;
    rght.x += dx;
    cloud->points.push_back(left);
    cloud->points.push_back(rght);
  }

  // create hull
  PointCloud<PointXYZ>::Ptr hullCloud(new PointCloud<PointXYZ>);
  vector<Vertices> rings(4);
  float radiuses[] = {rMin - 0.01f, rMax + 0.01f, rMin - 0.01f, rMax + 0.01f};
  float centers[] = {-dx, -dx, +dx, +dx};
  for (size_t i = 0; i < rings.size(); i++) {
    auto r = radiuses[i];
    auto xCenter = centers[i];
    for (float a = -M_PI; a < M_PI; a += 0.05f) {
      rings[i].vertices.push_back(hullCloud->size());
      hullCloud->push_back({xCenter + r * std::cosf(a), r * std::sinf(a), 0});
    }
  }

  // add more points before using prism
  size_t ringsPointCount = cloud->size();
  cloud->points.push_back({0, 0, 0});
  for (float a = -M_PI; a < M_PI; a += 0.05f) {
    float r = 4 * rMax;
    cloud->points.push_back({r * std::cosf(a), r * std::sinf(a), 0});
  }

  // do prism
  PointIndices::Ptr inliers(new PointIndices);
  ExtractPolygonalPrismData<PointXYZ> ex;
  {
    ex.setInputCloud(cloud);
    ex.setInputPlanarHull(hullCloud);
    ex.setHeightLimits(-1, 1);
    ex.setRings(rings);
    ex.segment(*inliers);
  }

  // check that all of the rings are in the prism.
  EXPECT_EQ(inliers->indices.size(), ringsPointCount);
}

int
main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return (RUN_ALL_TESTS());
}
