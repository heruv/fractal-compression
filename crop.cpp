#include "crop.h"
#include <random>

Crop::Crop(string path_to_img, int domain_size, int rang_size)
{
    input_img_ = imread(path_to_img, IMREAD_GRAYSCALE);

    if (input_img_.empty())
    {
        cout << "Unable to read image" << std::endl;
        abort();
    }

    domain_bloc_size_ = domain_size;
    rang_bloc_size_ = rang_size;

    int new_rows = input_img_.rows - (input_img_.rows % domain_bloc_size_);
    int new_cols = input_img_.cols - (input_img_.cols % domain_bloc_size_);

    input_img_ = input_img_(Rect(0, 0, new_cols, new_rows));
    output_img_.create(new_rows, new_cols, CV_8UC1);


    split();

    makeTreads();

    merge();
    cout << output_img_.size() << endl << rois_.size() << endl;
    imshow("merge", output_img_);
    waitKey(0);
}

void Crop::split()
{
    for (int i = 0; i < input_img_.rows; i += rang_bloc_size_)
    {
        for (int j = 0; j < input_img_.cols; j += rang_bloc_size_)
        {
            Mat block = input_img_(Rect(j, i, rang_bloc_size_, rang_bloc_size_));
            rois_.push_back(block);
        }
    }

    for (int i = 0; i < input_img_.rows; i += domain_bloc_size_)
    {
        for (int j = 0; j < input_img_.cols; j += domain_bloc_size_)
        {
            Mat block = input_img_(Rect(j, i, domain_bloc_size_, domain_bloc_size_));
            domain_rois_.push_back(block);
        }
    }
}

void Crop::merge()
{
    int n = 0;
    // so far, we are merging image into new one

    for (int i = 0; i < output_img_.rows; i += rang_bloc_size_)
    {
        for (int j = 0; j < output_img_.cols; j += rang_bloc_size_)
        {
            rois_.at(n).copyTo(output_img_(Rect(j, i, rang_bloc_size_, rang_bloc_size_)));
            n++;
        }
    }
}

Mat Crop::contractive(Mat&block, tuple<float,float> coef, float angle = 90.0f )
{
    Mat result;

    Mat flip   = Crop::flip(block, 1);
    Mat rotate = Crop::rotate(flip, angle);

    rotate.convertTo(result, CV_8UC1, get<0>(coef), get<1>(coef));
    cout << "after flip: " << endl;
    cout << result << endl;

    return result;
}

Mat Crop::reduceDomain(Mat&to_reduce)
{
        Mat reduce;
        resize(to_reduce, reduce, rois_[0].size());
        
        cout << "after_resize: " << endl;
        cout << reduce << endl;

        return reduce;
}

Mat Crop::rotate(Mat&to_rotate, float angle)
{
    Mat rotate;
    Mat rotated= getRotationMatrix2D(Point((domain_rois_.at(0).cols -1)/ 2.0f, (domain_rois_.at(0).rows -1)/ 2.0f), angle, 1.0);
    warpAffine(to_rotate, rotate, rotated, domain_rois_.at(0).size());

    return rotate;
}

Mat Crop::flip(Mat&to_flip, int direction)
{
    Mat flipped;
    cv::flip(to_flip, flipped, 1);

    return flipped;
}

tuple<float,float> Crop::findContrastBrightness(Mat&domain_block, Mat&range_block)
{
    if (!domain_block.isContinuous()) {
        domain_block = domain_block.clone();
    }

    if (!range_block.isContinuous()) {
        range_block = range_block.clone();
    }

    Mat domain_reshaped = domain_block.reshape(1, domain_block.total());
    Mat range_reshaped = range_block.reshape(1, range_block.total());

    Mat A(domain_reshaped.rows, 2, CV_8UC1);
    domain_reshaped.copyTo(A.col(0));
    A.col(1) = Mat::ones(domain_reshaped.rows, 1, CV_8UC1);


    cout << A << endl;

    Mat x;
    solve(A, range_reshaped, x, DECOMP_SVD);

    cout << x.at<double>(0) << x.at<float>(1);

    return {x.at<double>(0), x.at<float>(1)};
}

void Crop::findBestRatio(int area_size, int start_pos)
{
    array<int, 4> numbers = { 0, 1, 2, 3 };
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(0, 3);

    // Create thread-local storage for ratios to avoid data races
    vector<BestTransform> local_ratios;
    BestTransform local_ratio;

    for (int i = start_pos; i < start_pos + area_size; i++)
    {
        float error = INFINITY;

        for (int j = 0; j < domain_rois_.size(); j++)
        {
            int index = dist(gen);
            cout << 1 << endl;

            Mat temp = reduceDomain(domain_rois_[j]);
            cout << 2 << endl;

            auto factor = findContrastBrightness(temp, rois_[i]);
            cout << 3 << endl;

            Mat contractive = Crop::contractive(domain_rois_[j], factor, index * 90.0f);
            cout << 4 << endl;

            float current_error = findError(contractive, rois_[i]);
            if (current_error < error)
            {
                error = current_error;
                local_ratio.roi_index = i;
                local_ratio.direction = 1;
                local_ratio.factor = factor;
                local_ratio.rotation_angle = index * 90.0f;
                local_ratio.index = j;
            }
        }

            local_ratios.push_back(local_ratio);

            {
                lock_guard<mutex> lock(cout_mutex);  // You'll need to declare cout_mutex as a class member
                cout << "index: " << i << endl;
                printBestTransform(local_ratio);
            }
    }

   

    // Merge thread-local results with main storage
    lock_guard<mutex> lock(ratio_mutex);  
    total_ratio_.insert(total_ratio_.end(), local_ratios.begin(), local_ratios.end()); 
}

float Crop::findError(Mat& domain_block, Mat& range_block)
{
    float error = 0.0f;

    /*cout << domain_block << endl << endl;
    cout << range_block << endl << endl;
    cout << domain_block.size() << endl;*/
    Sleep(1000);

    for (int i = 0; i < domain_block.rows; i++)
    {
        for (int j = 0; j < domain_block.cols; j++)
        {
            float D = domain_block.at<uchar>(i, j);
            float R = range_block.at<uchar>(i, j);

            error += R - D * D;
        }
    }

    return abs(error);
}

void Crop::makeTreads()
{
    int area_size = rois_.size() / 1;

    vector<thread> thr;

    for (int i = 0, start_position = 0; i < 1; i++, start_position+=area_size)
    {
        cout << "i: " << i << endl;
        thr.emplace_back(&Crop::findBestRatio, this, area_size, start_position);
    }

    for (auto& t : thr)
    {
        t.join();
    }

    cout << total_ratio_.size();
    cout << "done";
}
