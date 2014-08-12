// IRTrack.cpp : Defines the entry point for the console application.
//
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <gl\gl.h>		
#include <gl\glu.h>	
#include <iostream>
#include <fstream>

#include "opencv2\core\core.hpp"
#include "opencv2\highgui\highgui.hpp"
#include "opencv2\imgproc\imgproc.hpp"

#include "cameramanager.h"
#include "cameralibrary.h"

using namespace cv;
using namespace CameraLibrary;

//******GLOBALS
Camera *camera[kMaxCameras];
int exposure;
int thresholdVal;
int slider;
int intensity;
int slider2;
int cameraCount = 0;


//************COMMON FUNCTIONS
void changeExp(int, void*)
{
	camera[0]->SetExposure(exposure);
}
void changeInt(int, void*)
{
	camera[0]->SetIntensity(intensity);
}
void changeThre(int, void*)
{
	camera[0]->SetThreshold(thresholdVal);
}
int _tmain(int argc, _TCHAR* argv[])
{
	printf("==============================================================================\n");
	printf("== DEFECT DETECTION                                TARAMBOLA DEVELOPMENT ==\n");
	printf("==============================================================================\n\n");

	printf("Waiting for cameras to spin up...");

	//== Initialize Camera Library ==----
	CameraLibrary_EnableDevelopment();

	CameraManager::X().WaitForInitialization();

	//== Verify all cameras are initialized ==--

	if (CameraManager::X().AreCamerasInitialized())
		printf("complete\n\n");
	else
		printf("failed\n\n");

	CameraList list;
	printf("%d", list.Count());
	for (int i = 0; i < list.Count(); i++)
	{
		printf("Device %d: %s", i, list[i].Name());
		camera[i] = CameraManager::X().GetCamera(list[i].UID());
		if (camera[i] == 0)
			printf("Unable to connect camera...");
		else
		{
			
			cameraCount++;
		}

	}
	
	/*if (cameraCount == 0)
	{
		MessageBox(0, "Please connect a camera", "No Device Connected", MB_OK);
		CameraManager::X().Shutdown();
		return 1;
	}*/
	camera[0] = CameraManager::X().GetCamera();
	int cameraWidth = camera[0]->Width();
	int cameraHeight = camera[0]->Height();
	std::cout << camera[0]->Exposure();
	exposure = int(camera[0]->Exposure());
	intensity = camera[0]->Intensity();
	thresholdVal = int(camera[0]->Threshold());

	uchar imageBuffer[800 * 600];


	for (int i = 0; i < cameraCount; i++)
	{
		camera[i]->SetVideoType(SegmentMode);
		//camera->SetVideoType(MJPEGMode);
		camera[i]->Start();
		camera[i]->SetTextOverlay(false);
	}
	camera[0]->SetVideoType(SegmentMode);
	//camera->SetVideoType(MJPEGMode);
	camera[0]->Start();
	camera[0]->SetTextOverlay(false);
	

	Core::DistortionModel lensDistortion;
	camera[0]->GetDistortionModel(lensDistortion);

	//************ WINDOW **************
	namedWindow("IRTRACK", CV_WINDOW_AUTOSIZE);
	namedWindow("BLOB", CV_WINDOW_AUTOSIZE);
	createTrackbar("THRESHOLD", "IRTRACK", &thresholdVal, camera[0]->MaximumThreshold(), changeThre);
	createTrackbar("INTENSITY", "IRTRACK", &intensity, camera[0]->MaximumIntensity(), changeInt);
	createTrackbar("EXPOSURE", "IRTRACK", &exposure, camera[0]->MaximumExposureValue(), changeExp);
	//namedWindow("CONTOUR", CV_WINDOW_AUTOSIZE);
	//namedWindow("CONTOUR2", CV_WINDOW_AUTOSIZE);
	//************ @WINDOW **************
	//*************IMAGE ***************
	IplImage *img = cvCreateImage(cvSize(cameraWidth, cameraHeight), IPL_DEPTH_8U, 1);
	//IplImage *imgRGB = cvCreateImage(cvSize(cameraWidth, cameraHeight), IPL_DEPTH_8U, 3);
	Mat imgRGB;
	imgRGB = Mat(cameraWidth, cameraHeight, CV_8UC3);
//	imgRGB = imread("img.jpg", 1);
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	Mat canny_output;
	Mat src; Mat src_gray;
	int thresh = 100;
	int max_thresh = 255;
	RNG rng(12345);
	src = imread("img.jpg", 1);
	cvtColor(src, src_gray, CV_BGR2GRAY);
	blur(src_gray, src_gray, Size(3, 3));
	Canny(src_gray, canny_output, thresh, thresh * 2, 3);
	findContours(canny_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
	Mat drawing = Mat::zeros(canny_output.size(), CV_8UC3);
	for (int i = 0; i< contours.size(); i++)
	{
		Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
		drawContours(drawing, contours, i, color, 2, 8, hierarchy, 0, Point());
	}

	while (1)
	{	
		Frame *frame = camera[0]->GetFrame();
		if (frame)
		{
			frame->Rasterize(cameraWidth, cameraHeight, cameraWidth, 8, imageBuffer);

			img->imageData=(char *)imageBuffer;
			//imgRGB.data = imageBuffer;
			//cvCvtColor(img, imgRGB, CV_GRAY2BGR);
			cvFlip(img, NULL, 1);
			imgRGB.setTo(0);
			

			for (int i = 0; i<frame->ObjectCount(); i++)
			{
				cObject *obj = frame->Object(i);

				float x = obj->X();
				float y = obj->Y();

				int a = cvRound(x);
				int b = cvRound(y);
				circle(imgRGB, cvPoint(cameraWidth-x, y), 20, CV_RGB(255, 0, 0));
				char c[30] = "P(";
				char cx[5] = "100";
				char cy[5] = "100";
				sprintf_s(cx, "%d", a);
				sprintf_s(cy, "%d", b);
				strcat_s(c, cx);
				strcat_s(c, ", ");
				strcat_s(c, cy);
				strcat_s(c, ")");
				//putText(imgRGB, c, cvPoint(x, y), 1,1, cvScalar(255.0, 0.0, 0.0, 0.0));
				//flip(imgRGB, imgRGB, 1);
				putText(imgRGB, c, cvPoint(cameraWidth-a, b-20), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(0, 250, 0), 1, CV_AA);

				Core::Undistort2DPoint(lensDistortion, x, y);
			}
			frame->Release();
		}
		//Sleep(2);

		//»»»»»»»»»»»»» WINDOW 
		cvShowImage("BLOB", img);
		imshow("IRTRACK", imgRGB);
	//	imshow("CONTOUR", drawing);
	//	imshow("CONTOUR2", canny_output);

		int key = waitKey(10);
		if (key == 27) break;

	}

	CameraManager::X().Shutdown();

	return 0;
}