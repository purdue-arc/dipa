/*
 * FeatureTracker.cpp
 *
 *  Created on: Jul 8, 2017
 *      Author: kevin
 */

#include <dipa/planar_odometry/FeatureTracker.h>

FeatureTracker::FeatureTracker() {
	// TODO Auto-generated constructor stub

}

FeatureTracker::~FeatureTracker() {
	// TODO Auto-generated destructor stub
}

void FeatureTracker::updateFeatures(cv::Mat img)
{

	std::vector<cv::Point2f> oldPoints = this->state.getPixels2fInOrder();

	ROS_ASSERT(oldPoints.size() > 0);

	//ROS_DEBUG_STREAM_ONCE("got " << oldPoints.size() << " old point2fs from the oldframe which has " << oldFrame.features.size() << " features");
	std::vector<cv::Point2f> newPoints;

	std::vector<uchar> status; // status vector for each point
	cv::Mat error; // error vector for each point

	ROS_DEBUG("before klt");

	cv::calcOpticalFlowPyrLK(this->state.currentImg, img, oldPoints, newPoints, status, error, cv::Size(21, 21), 3,
			cv::TermCriteria(cv::TermCriteria::COUNT+cv::TermCriteria::EPS, 30, 0.01), 0, KLT_MIN_EIGEN);

	ROS_DEBUG("after klt");

	std::vector<Feature> flowedFeatures;

	int lostFeatures = 0;

	for(int i = 0; i < status.size(); i++)
	{
		if(status.at(i) == 1)
		{
			Feature updated_feature = this->state.features.at(i);

			updated_feature.px = newPoints.at(i);

			flowedFeatures.push_back(updated_feature);

		}
		else
		{
			lostFeatures++;
		}
	}

	this->state.features = flowedFeatures;

	this->state.currentImg = img; // the new image is now the old image

	ROS_DEBUG_STREAM("VO LOST " << lostFeatures << "FEATURES");

}

bool FeatureTracker::computePose(double& perPixelError)
{
	return true;
}

void FeatureTracker::updatePose(tf::Transform w2c)
{
	this->state.currentPose = w2c; // set the new pose

	this->state.updateObjectPositions(this->K); // update the object positions to eliminate the drift
}

/*
 * get more features after updating the pose
 */
void FeatureTracker::replenishFeatures(cv::Mat img)
{
	//add more features if needed
	if(this->state.features.size() < NUM_FEATURES)
	{
		std::vector<cv::KeyPoint> fast_kp;
		cv::FAST(img, fast_kp, FAST_THRESHOLD, true);

		int needed = NUM_FEATURES - this->state.features.size();

		//TODO check if feature already exists

		for(int i = 0; i < needed && i < fast_kp.size(); i++)
		{
			Feature new_ft;

			new_ft.px = fast_kp.at(i).pt;

			new_ft.computeObjectPosition(this->state.currentPose, this->K); // corresponf to a 3d point

			this->state.features.push_back(new_ft);

		}
	}

	this->state.currentImg = img;

#if SUPER_DEBUG

	cv::Mat copy = img.clone();

	copy = this->draw(copy);

	cv::imshow("vo", copy);
	cv::waitKey(30);

#endif
}
