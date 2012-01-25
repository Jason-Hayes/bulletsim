#include <iostream>
#include <string>
#include "comm/comm2.h"
#include "comm/comm_eigen.h"

#include "comm_pcl.h"
#include "comm_cv.h"
#include "cloud_filtering.h"
#include "vector_io.h"
#include "my_exceptions.h"
#include <boost/program_options.hpp>
#include <opencv2/imgproc/imgproc.hpp>


namespace po = boost::program_options;
using namespace std;

int main(int argc, char* argv[]) {

  string cloudTopic;
  string labelTopic;
  string outTopic;
  bool doPause;
  int label;
  float voxelSize;
  int erosion;

  po::options_description opts("Allowed options");
  opts.add_options()
    ("help,h", "produce help message")
    ("cloudTopic,c", po::value< string >(&cloudTopic)->default_value("kinect"),"cloud topic")
    ("labelTopic,l", po::value< string >(&labelTopic),"label topic")
    ("outTopic,o", po::value< string >(&outTopic),"output topic")
    ("label,n", po::value<int>(&label)->default_value(1),"label to extract")
    ("voxelSize,v", po::value<float>(&voxelSize)->default_value(.01),"voxel side length (meters)")
    ("erosion,e", po::value<int>(&erosion)->default_value(1),"radius for morphological erosion of region");
  po::variables_map vm;        
  po::store(po::command_line_parser(argc, argv)
	    .options(opts)
	    .run()
	    , vm);
  if (vm.count("help")) {
    cout << "usage: comm_downsample_clouds [options]" << endl;
    cout << opts << endl;
    return 0;
  }
  po::notify(vm);

  setDataRoot();

  CloudMessage cloudMsg;
  ImageMessage labelMsg;
  FileSubscriber cloudSub(cloudTopic,"pcd");
  FileSubscriber labelSub(labelTopic,"png");
  FilePublisher cloudPub(outTopic,"pcd");

  cv::Mat sel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(erosion, erosion));

  while (true) {
    bool gotOne = cloudSub.recv(cloudMsg);
    if (!gotOne) break;

    gotOne = labelSub.recv(labelMsg);
    assert(gotOne);

    vector<cv::Mat> channels;
    cv::split(labelMsg.m_data,channels);
    cv::Mat mask = channels[0] == label;
    cv::Mat erodedMask;
    erode(mask, erodedMask, sel);

    ColorCloudPtr maskedCloud = maskCloud(cloudMsg.m_data, erodedMask);
    ColorCloudPtr downedCloud = downsampleCloud(maskedCloud, voxelSize);
    ColorCloudPtr finalCloud = removeOutliers(downedCloud);

    cloudPub.send(CloudMessage(finalCloud));

  }


}
