#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include "crop.h"

using namespace cv;
using namespace std;

int main()
{
    string path;

    cout << "enter full path to image: ";
    cin >> path;

    Crop img(path, 8 ,4);



    return 0;
}