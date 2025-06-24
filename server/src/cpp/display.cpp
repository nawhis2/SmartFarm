#include "main.h"
#include <opencv2/opencv.hpp>

using namespace cv;

void display_image(const char *filepath)
{
    Mat img = imread(filepath);
    if (img.empty()) {
        printf("OpenCV failed to read image: %s\n", filepath);
        return;
    }

    imshow("Received Image", img);
    waitKey(0);
    destroyAllWindows();
}