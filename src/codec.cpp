

#include "codec.h"

double getPSNR(const cv::Mat& I1, const cv::Mat& I2)
{
    cv::Mat s1;
    absdiff(I1, I2, s1);       // |I1 - I2|
    s1.convertTo(s1, CV_32F);  // cannot make a square on 8 bits
    s1 = s1.mul(s1);           // |I1 - I2|^2

    cv::Scalar s = sum(s1);         // sum elements per channel

    double sse = s.val[0] + s.val[1] + s.val[2]; // sum channels

    if (sse <= 1e-10) // for small values return zero
        return 0;
    else
    {
        double  mse = sse / (double)(I1.channels() * I1.total());
        double psnr = 10.0 * log10((255 * 255) / mse);
        return psnr;
    }
}


cv::Mat lossy_bw_limit(cv::Mat& input_img, double psnr)
{
    std::string suff(".jpg"); // target format
    if (!cv::haveImageWriter(suff))
        throw std::runtime_error("Can not compress to format:" + suff);

    std::vector<uchar> bytes;
    std::vector<int> compression_params;

    // prepare parameters for JPEG compressor
    // we use only quality, but other parameters are available (progressive, optimization...)
    std::vector<int> compression_params_template;
    compression_params_template.push_back(cv::IMWRITE_JPEG_QUALITY);

    std::cout << '[';
    cv::Mat decoded_frame;
    //try step-by-step to decrease quality by 5%, until it fits into limit
    for (auto i = 100; i > 0; i -= 5) {
        compression_params = compression_params_template; // reset parameters
        compression_params.push_back(i);                  // set desired quality

        // try to encode
        cv::imencode(suff, input_img, bytes, compression_params);
        decoded_frame = cv::imdecode(bytes, cv::IMREAD_ANYCOLOR);
        // check the size limit
        double actualPSNR = getPSNR(input_img, decoded_frame) / 100.0;
        std::cout << i << ':' << actualPSNR << ',';
        if (actualPSNR <= psnr)
            break; // ok, done 
    }
    std::cout << "]\n";

    return decoded_frame;
}

void lossyEncodeAsync(cv::Mat& frame, cv::Mat& encodeFrame, bool& appClosed, std::mutex& mutex, float& compressionQuality)
{
    std::string suff(".jpg"); // target format
    if (!cv::haveImageWriter(suff))
        throw std::runtime_error("Can not compress to format:" + suff);

    cv::Mat input_img, encoded_frame;
    std::vector<uchar> bytes;
    std::vector<int> compression_params;

    // prepare parameters for JPEG compressor
    // we use only quality, but other parameters are available (progressive, optimization...)
    std::vector<int> compression_params_template;
    compression_params_template.push_back(cv::IMWRITE_JPEG_QUALITY);

    while (!appClosed) {

        {
            std::scoped_lock lock(mutex);
            frame.copyTo(input_img);
        }


        compression_params = compression_params_template; // reset parameters
        compression_params.push_back(compressionQuality);                  // set desired quality

        // try to encode
        if (!input_img.empty())
        {
            cv::imencode(suff, input_img, bytes, compression_params);
            encoded_frame = cv::imdecode(bytes, cv::IMREAD_ANYCOLOR);

            {
                std::scoped_lock lock(mutex);
                encoded_frame.copyTo(encodeFrame);
            }
        }
    }
}