///////////////////////////////////////////////////////////////////////////////
//
//      TargaImage.cpp                          Author:     Stephen Chenney
//                                              Modified:   Eric McDaniel
//                                              Date:       Fall 2004
//
//      Implementation of TargaImage methods.  You must implement the image
//  modification functions.
//
///////////////////////////////////////////////////////////////////////////////

#include "Globals.h"
#include "TargaImage.h"
#include "libtarga.h"
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <math.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>

using namespace std;

// constants
const int           RED             = 0;                // red channel
const int           GREEN           = 1;                // green channel
const int           BLUE            = 2;                // blue channel
const unsigned char BACKGROUND[3]   = { 0, 0, 0 };      // background color


// Computes n choose s, efficiently
double Binomial(int n, int s)
{
    double        res;

    res = 1;
    for (int i = 1 ; i <= s ; i++)
        res = (n - i + 1) * res / i ;

    return res;
}// Binomial


///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage() : width(0), height(0), data(NULL)
{}// TargaImage

///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(int w, int h) : width(w), height(h)
{
   data = new unsigned char[width * height * 4];
   ClearToBlack();
}// TargaImage



///////////////////////////////////////////////////////////////////////////////
//
//      Constructor.  Initialize member variables to values given.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(int w, int h, unsigned char *d)
{
    int i;

    width = w;
    height = h;
    data = new unsigned char[width * height * 4];

    for (i = 0; i < width * height * 4; i++)
	    data[i] = d[i];
}// TargaImage

///////////////////////////////////////////////////////////////////////////////
//
//      Copy Constructor.  Initialize member to that of input
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::TargaImage(const TargaImage& image) 
{
   width = image.width;
   height = image.height;
   data = NULL; 
   if (image.data != NULL) {
      data = new unsigned char[width * height * 4];
      memcpy(data, image.data, sizeof(unsigned char) * width * height * 4);
   }
}


///////////////////////////////////////////////////////////////////////////////
//
//      Destructor.  Free image memory.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage::~TargaImage()
{
    if (data)
        delete[] data;
}// ~TargaImage


///////////////////////////////////////////////////////////////////////////////
//
//      Converts an image to RGB form, and returns the rgb pixel data - 24 
//  bits per pixel. The returned space should be deleted when no longer 
//  required.
//
///////////////////////////////////////////////////////////////////////////////
unsigned char* TargaImage::To_RGB(void)
{
    unsigned char   *rgb = new unsigned char[width * height * 3];
    int		    i, j;

    if (! data)
	    return NULL;

    // Divide out the alpha
    for (i = 0 ; i < height ; i++)
    {
	    int in_offset = i * width * 4;
	    int out_offset = i * width * 3;

	    for (j = 0 ; j < width ; j++)
        {
	        RGBA_To_RGB(data + (in_offset + j*4), rgb + (out_offset + j*3));
	    }
    }

    return rgb;
}// TargaImage


///////////////////////////////////////////////////////////////////////////////
//
//      Save the image to a targa file. Returns 1 on success, 0 on failure.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Save_Image(const char *filename)
{
    TargaImage	*out_image = Reverse_Rows();

    if (! out_image)
	    return false;

    if (!tga_write_raw(filename, width, height, out_image->data, TGA_TRUECOLOR_32))
    {
	    cout << "TGA Save Error: %s\n", tga_error_string(tga_get_last_error());
	    return false;
    }

    delete out_image;

    return true;
}// Save_Image


///////////////////////////////////////////////////////////////////////////////
//
//      Load a targa image from a file.  Return a new TargaImage object which 
//  must be deleted by caller.  Return NULL on failure.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage* TargaImage::Load_Image(char *filename)
{
    unsigned char   *temp_data;
    TargaImage	    *temp_image;
    TargaImage	    *result;
    int		        width, height;

    if (!filename)
    {
        cout << "No filename given." << endl;
        return NULL;
    }// if

    temp_data = (unsigned char*)tga_load(filename, &width, &height, TGA_TRUECOLOR_32);
    if (!temp_data)
    {
        cout << "TGA Error: %s\n", tga_error_string(tga_get_last_error());
	    width = height = 0;
	    return NULL;
    }
    temp_image = new TargaImage(width, height, temp_data);
    free(temp_data);

    result = temp_image->Reverse_Rows();

    delete temp_image;

    return result;
}// Load_Image


///////////////////////////////////////////////////////////////////////////////
//
//      Convert image to grayscale.  Red, green, and blue channels should all 
//  contain grayscale value.  Alpha channel shoould be left unchanged.  Return
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::To_Grayscale()
{
    for (int i = 0; i < width * height * 4; i += 4)
    {
        unsigned char gray = 0.299 * data[i] + 0.587 * data[i + 1] + 0.114 * data[i + 2];

        for (int j = 0; j < 3; j++) {
            data[i + j] = gray;
        }
    }

    return true;
	//ClearToBlack();
	//return false;
}// To_Grayscale


///////////////////////////////////////////////////////////////////////////////
//
//  Convert the image to an 8 bit image using uniform quantization.  Return 
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Quant_Uniform()
{
    for (int i = 0; i < width * height * 4; i += 4)
    {
        data[i] = data[i] >> 5;     //red 3 bits
        data[i] = data[i] << 5;

        data[i + 1] = data[i + 1] >> 5;     //green 3 bits
        data[i + 1] = data[i + 1] << 5;

        data[i + 2] = data[i + 2] >> 6;     //blue 2 bits
        data[i + 2] = data[i + 2] << 6;
    }

    return true;
    //ClearToBlack();
    //return false;
}// Quant_Uniform


///////////////////////////////////////////////////////////////////////////////
//
//      Convert the image to an 8 bit image using populosity quantization.  
//  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Quant_Populosity()
{
    ClearToBlack();
    return false;
}// Quant_Populosity


///////////////////////////////////////////////////////////////////////////////
//
//      Dither the image using a threshold of 1/2.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Threshold()
{
    for (int h = 0; h < height; h++) {
        int offset = h * width * 4;     //length of one row

        for (int w = 0; w < width; w++) {

            if ((float)(data[offset + w * 4]+ data[offset + w * 4 + 1] + data[offset + w * 4 + 2]) / 3 / 256.0 >= 0.5)
                data[offset + w * 4] = data[offset + w * 4 + 1] = data[offset + w * 4 + 2] = 255;
            else
                data[offset + w * 4] = data[offset + w * 4 + 1] = data[offset + w * 4 + 2] = 0;

        }
    }

    return true;

    //ClearToBlack();
    //return false;
}// Dither_Threshold


///////////////////////////////////////////////////////////////////////////////
//
//      Dither image using random dithering.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Random()
{
    // compute average intensity
    float sum = 0;
    float intensity = 0;
    vector<float> sortedData;

    for (int h = 0; h < height; h++) {
        int offset = h * width * 4;     //length of one row

        for (int w = 0; w < width; w++) {

            intensity = 0.299 * data[offset + w * 4] + 0.587 * data[offset + w * 4 + 1] + 0.114 * data[offset + w * 4 + 2];
            sortedData.push_back(intensity);
            sum += intensity;
        }
    }
    float avg = sum / (width * height) / 256;
    float thresh = sum * avg * 2.5;
    sort(sortedData.begin(), sortedData.end());

    //cout << "avg: " << avg << endl;
    //cout << "sum thresh: " << thresh << endl;

    // find threshold
    sum = 0;
    for (int i = 0; i < sortedData.size(); i++) {
        sum += sortedData[i];

        if (sum >= thresh) {
            thresh = sortedData[i];
            break;
        }
    }
    //cout << "sum: " << sum << endl;
    //cout << "thresh: " << thresh/256 << endl;

    // 
    for (int h = 0; h < height; h++) {
        int offset = h * width * 4;     //length of one row

        for (int w = 0; w < width; w++) {
            float random = (float)(-2 + rand() % 5) / 10.0;

            intensity = 0.299 * data[offset + w * 4] + 0.587 * data[offset + w * 4 + 1] + 0.114 * data[offset + w * 4 + 2];
            intensity /= 256.0;
            intensity += random;

            if (intensity >= thresh / 256.0)
                data[offset + w * 4] = data[offset + w * 4 + 1] = data[offset + w * 4 + 2] = 255;
            else
                data[offset + w * 4] = data[offset + w * 4 + 1] = data[offset + w * 4 + 2] = 0;

        }
    }


    return true;
    //ClearToBlack();
    //return false;
}// Dither_Random


///////////////////////////////////////////////////////////////////////////////
//
//      Perform Floyd-Steinberg dithering on the image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_FS()
{
    ClearToBlack();
    return false;
}// Dither_FS


///////////////////////////////////////////////////////////////////////////////
//
//      Dither the image while conserving the average brightness.  Return 
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Bright()
{
    // compute average intensity
    float sum = 0;
    float intensity = 0;
    vector<float> sortedData;

    for (int h = 0; h < height; h++) {
        int offset = h * width * 4;     //length of one row

        for (int w = 0; w < width; w++) {

            intensity = 0.299 * data[offset + w * 4] + 0.587 * data[offset + w * 4 + 1] + 0.114 * data[offset + w * 4 + 2];
            sortedData.push_back(intensity);
            sum += intensity;
        }
    }
    float avg = sum / (width * height) / 256;
    float thresh = sum * avg * 2.5;
    sort(sortedData.begin(), sortedData.end());

    //cout << "avg: " << avg << endl;
    //cout << "sum thresh: " << thresh << endl;

    // find threshold
    sum = 0;
    for (int i = 0; i < sortedData.size(); i++) {
        sum += sortedData[i];

        if (sum >= thresh) {
            thresh = sortedData[i];
            break;
        }
    }
    //cout << "sum: " << sum << endl;
    //cout << "thresh: " << thresh/256 << endl;

    // 
    for (int h = 0; h < height; h++) {
        int offset = h * width * 4;     //length of one row

        for (int w = 0; w < width; w++) {
            intensity = 0.299 * data[offset + w * 4] + 0.587 * data[offset + w * 4 + 1] + 0.114 * data[offset + w * 4 + 2];

            if (intensity >= thresh)
                data[offset + w * 4] = data[offset + w * 4 + 1] = data[offset + w * 4 + 2] = 255;
            else
                data[offset + w * 4] = data[offset + w * 4 + 1] = data[offset + w * 4 + 2] = 0;

        }
    }


    return true;

    //ClearToBlack();
    //return false;
}// Dither_Bright


///////////////////////////////////////////////////////////////////////////////
//
//      Perform clustered differing of the image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Cluster()
{
    float mask[4][4] = {
        {0.7059, 0.3529, 0.5882, 0.2353},
        {0.0588, 0.9412, 0.8235, 0.4118},
        {0.4706, 0.7647, 0.8824, 0.1176},
        {0.1765, 0.5294, 0.2941, 0.6471}
    };

    for (int h = 0; h < height; h++) {
        int offset = h * width * 4;     //length of one row

        for (int w = 0; w < width; w++) {

            if ((float)(data[offset + w * 4] + data[offset + w * 4 + 1] + data[offset + w * 4 + 2]) / 3 / 256.0 >= mask[h % 4][w % 4])
                data[offset + w * 4] = data[offset + w * 4 + 1] = data[offset + w * 4 + 2] = 255;
            else
                data[offset + w * 4] = data[offset + w * 4 + 1] = data[offset + w * 4 + 2] = 0;
        }
    }


    return true;
    //ClearToBlack();
    //return false;
}// Dither_Cluster


///////////////////////////////////////////////////////////////////////////////
//
//  Convert the image to an 8 bit image using Floyd-Steinberg dithering over
//  a uniform quantization - the same quantization as in Quant_Uniform.
//  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Dither_Color()
{
    ClearToBlack();
    return false;
}// Dither_Color


///////////////////////////////////////////////////////////////////////////////
//
//      Composite the current image over the given image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Over(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout <<  "Comp_Over: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_Over


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image "in" the given image.  See lecture notes for 
//  details.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_In(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_In: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_In


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image "out" the given image.  See lecture notes for 
//  details.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Out(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_Out: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_Out


///////////////////////////////////////////////////////////////////////////////
//
//      Composite current image "atop" given image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Atop(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_Atop: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_Atop


///////////////////////////////////////////////////////////////////////////////
//
//      Composite this image with given image using exclusive or (XOR).  Return
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Comp_Xor(TargaImage* pImage)
{
    if (width != pImage->width || height != pImage->height)
    {
        cout << "Comp_Xor: Images not the same size\n";
        return false;
    }

    ClearToBlack();
    return false;
}// Comp_Xor


///////////////////////////////////////////////////////////////////////////////
//
//      Calculate the difference bewteen this imag and the given one.  Image 
//  dimensions must be equal.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Difference(TargaImage* pImage)
{
    if (!pImage)
        return false;

    if (width != pImage->width || height != pImage->height)
    {
        cout << "Difference: Images not the same size\n";
        return false;
    }// if

    for (int i = 0 ; i < width * height * 4 ; i += 4)
    {
        unsigned char        rgb1[3];
        unsigned char        rgb2[3];

        RGBA_To_RGB(data + i, rgb1);
        RGBA_To_RGB(pImage->data + i, rgb2);

        data[i] = abs(rgb1[0] - rgb2[0]);
        data[i+1] = abs(rgb1[1] - rgb2[1]);
        data[i+2] = abs(rgb1[2] - rgb2[2]);
        data[i+3] = 255;
    }

    return true;
}// Difference

float TargaImage::findKernelValue(float* kernel, int x, int y, int kernel_width, int kernel_height) {
    vector<int> pattern;

    // horizonal
    int kernel_row = y * kernel_width;
    int kernel_col = kernel_width - 1 - x;
    pattern.push_back(kernel_row + kernel_col);

    //vertical
    kernel_row = (kernel_height - 1 - y) * kernel_width;
    kernel_col = x;
    pattern.push_back(kernel_row + kernel_col);

    if (kernel_width == kernel_height) {
        // diagonal 1
        kernel_row = x * kernel_width;
        kernel_col = y;
        pattern.push_back(kernel_row + kernel_col);

        // diagonal 2
        kernel_row = (kernel_height - 1 - x) * kernel_width;
        kernel_col = kernel_width - 1 - y;
        pattern.push_back(kernel_row + kernel_col);
    }
    else {
        int kernel_min = min(kernel_height, kernel_width);

        // diagonal 1
        kernel_row = (kernel_min - 1 - y) * kernel_width;
        kernel_col = x;
        pattern.push_back(kernel_row + kernel_col);

        // diagonal 2
        kernel_row = (kernel_min - 1 - y) * kernel_width;
        kernel_col = kernel_min - 1 - x;
        pattern.push_back(kernel_row + kernel_col);
    }

    // 
    for (int i = 0; i < pattern.size(); i++) {
        if (kernel[pattern[i]] != 9999.0)
            return kernel[pattern[i]];
    }
}

float TargaImage::filter_pixel(float* filter_matrix, float divide, int kernel_width, int kernel_height, int w, int h, int channel) {
    int y_min = -(kernel_height - 1) / 2;
    int y_max = kernel_height / 2;
    int x_min = -(kernel_width - 1) / 2;
    int x_max = kernel_width / 2;

    // initialize kernel matrix
    float* kernel = new float[kernel_width * kernel_height];
    for (int y = 0; y < kernel_height; y++) {
        for (int kernel_col = 0; kernel_col < kernel_width; kernel_col++) {
            int kernel_row = y * kernel_width;

            kernel[kernel_row + kernel_col] = 9999.0;
        }
    }

    // iterate the target kernel matrix, to fill existing data
    for (int y = y_min; y <= y_max; y++) {
        for (int x = x_min; x <= x_max; x++) {

            // when the position is in the image, fill in data
            if (h + y >= 0 && h + y < height &&
                w + x >= 0 && w + x < width) {

                int row = (h + y) * width * 4;
                int col = (w + x) * 4 + channel;

                int kernel_row = (y - y_min) * kernel_width;
                int kernel_col = x - x_min;

                kernel[kernel_row + kernel_col] = (float)data[row + col] * filter_matrix[kernel_row + kernel_col];
            }

        }
    }

    // fill in the missing data
    for (int y = 0; y < kernel_height; y++) {
        for (int kernel_col = 0; kernel_col < kernel_width; kernel_col++) {
            int kernel_row = y * kernel_width;

            if (kernel[kernel_row + kernel_col] == 9999.0) {
                kernel[kernel_row + kernel_col] = findKernelValue(kernel, kernel_col, y, kernel_width, kernel_height);
            }
        }
    }

    // sum up data in the kernel
    float sum = 0;
    for (int y = 0; y < kernel_height; y++) {
        for (int kernel_col = 0; kernel_col < kernel_width; kernel_col++) {
            int kernel_row = y * kernel_width;

            sum += kernel[kernel_row + kernel_col];

            if (kernel[kernel_row + kernel_col] == 9999.0)
                cout << "error: fail to update\n";
        }
    }

    return sum / divide;
}

void TargaImage::filter(float* filter_matrix, float divide, int kernel_size) {
    unsigned char* newdata = new unsigned char[width * height * 4];
    memcpy(newdata, data, width * height * 4);

    int min = -(kernel_size - 1) / 2;
    int max = kernel_size / 2;
    
    // iterate pixels of image
    for (int h = 0; h < height; h++) {
        for (int w = 0; w < width; w++) {

            // itertate RGB channel
            for (int i = 0; i < 3; i++) {

                // renew data
                int row = h * width * 4;
                int col = w * 4 + i;

                newdata[row + col] = filter_pixel(filter_matrix, divide, kernel_size, kernel_size, w, h, i);
            }
            
        }
    }

    memcpy(data, newdata, width* height * 4);
    delete[] newdata;
}

///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 box filter on this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Box()
{
    float filter_matrix[25] = {
        1, 1, 1, 1, 1,
        1, 1, 1, 1, 1,
        1, 1, 1, 1, 1,
        1, 1, 1, 1, 1,
        1, 1, 1, 1, 1
    };

    filter(filter_matrix, 25, 5);

    return true;
    //ClearToBlack();
    //return false;
}// Filter_Box


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 Bartlett filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Bartlett()
{
    float filter_matrix[25] = {
        1, 2, 3, 2, 1,
        2, 4, 6, 4, 2,
        3, 6, 9, 6, 3,
        2, 4, 6, 4, 2,
        1, 2, 3, 2, 1,
    };

    filter(filter_matrix, 81, 5);

    return true;
    //ClearToBlack();
    //return false;
}// Filter_Bartlett


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 Gaussian filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Gaussian()
{
    float filter_matrix[25] = {
        1, 4, 6, 4, 1,
        4, 16, 24, 16, 4,
        6, 24, 36, 24, 6,
        4, 16, 24, 16, 4,
        1, 4, 6, 4, 1,
    };

    filter(filter_matrix, 256, 5);

    return true;
    //ClearToBlack();
    //return false;
}// Filter_Gaussian

///////////////////////////////////////////////////////////////////////////////
//
//      Perform NxN Gaussian filter on this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////

bool TargaImage::Filter_Gaussian_N( unsigned int N )
{
    ClearToBlack();
   return false;
}// Filter_Gaussian_N


///////////////////////////////////////////////////////////////////////////////
//
//      Perform 5x5 edge detect (high pass) filter on this image.  Return 
//  success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Edge()
{
    ClearToBlack();
    return false;
}// Filter_Edge


///////////////////////////////////////////////////////////////////////////////
//
//      Perform a 5x5 enhancement filter to this image.  Return success of 
//  operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Filter_Enhance()
{
    ClearToBlack();
    return false;
}// Filter_Enhance


///////////////////////////////////////////////////////////////////////////////
//
//      Run simplified version of Hertzmann's painterly image filter.
//      You probably will want to use the Draw_Stroke funciton and the
//      Stroke class to help.
// Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::NPR_Paint()
{
    ClearToBlack();
    return false;
}



///////////////////////////////////////////////////////////////////////////////
//
//      Halve the dimensions of this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Half_Size()
{
    float filter_matrix[9] = {
        1, 2, 1,
        2, 4, 2,
        1, 2, 1
    };

    // halve the size
    unsigned char* newdata = new unsigned char[(height / 2) * (width / 2) * 4];
    //memset(newdata, 0, (height / 2) * (width / 2) * 4);

    for (int h = 0; h < height / 2; h++) {
        for (int w = 0; w < width / 2; w++) {

            //rgb channel
            for (int i = 0; i < 4; i++) {
                if (i == 3) {
                    newdata[h * (width / 2) * 4 + w * 4 + i] = data[(h * 2 * width * 4) + (w * 2 * 4) + i];
                }
                else {
                    newdata[h * (width / 2) * 4 + w * 4 + i] = filter_pixel(filter_matrix, 16, 3, 3, w * 2, h * 2, i);
                }
            }
        }
    }

    //delete data;
    //data = newdata;
    memcpy(data, newdata, (height / 2) * (width / 2) * 4);
    height /= 2;
    width /= 2;

    delete[] newdata;
    return true;
    //ClearToBlack();
    //return false;
}// Half_Size


///////////////////////////////////////////////////////////////////////////////
//
//      Double the dimensions of this image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Double_Size()
{
    unsigned char* newdata = new unsigned char[height * 2 * width * 2 * 4];

    for (int newH = 0; newH < height * 2; newH++) {
        for (int newW = 0; newW < width * 2; newW++) {

            // RGBA channel
            for (int i = 0; i < 4; i++) {
                int row = newH / 2 * width * 4;
                int col = newW / 2 * 4 + i;
                int newRow = newH * width * 2 * 4;
                int newCol = newW * 4 + i;

                // alpha channel
                if (i == 3) {
                    newdata[newRow + newCol] = data[row + col];
                    continue;
                }

                // RGB
                if (newH % 2 == 0 && newW % 2 == 0) {
                    float filter_matrix[9] = {
                        1, 2, 1,
                        2, 4, 2,
                        1 ,2, 1
                    };

                    newdata[newRow + newCol] = filter_pixel(filter_matrix, 16, 3, 3, newW / 2, newH / 2, i);
                }
                else if (newH % 2 == 1 && newW % 2 == 1) {
                    float filter_matrix[16] = {
                        1, 3, 3, 1,
                        3, 9, 9, 3,
                        3, 9, 9, 3,
                        1, 3, 3, 1
                    };

                    newdata[newRow + newCol] = filter_pixel(filter_matrix, 64, 4, 4, newW / 2, newH / 2, i);
                }
                else if (newH % 2 == 1 && newW % 2 == 0) {
                    float filter_matrix[12] = {
                        1, 2, 1,
                        3, 6, 3,
                        3, 6, 3,
                        1, 2, 1
                    };

                    newdata[newRow + newCol] = filter_pixel(filter_matrix, 32, 3, 4, newW / 2, newH / 2, i);
                }
                else {
                    float filter_matrix[12] = {
                        1, 3, 3, 1,
                        2, 6, 6, 2,
                        1, 3, 3, 1
                    };

                    newdata[newRow + newCol] = filter_pixel(filter_matrix, 32, 4, 3, newW / 2, newH / 2, i);
                }
            }
        }
    }

    delete data;
    data = newdata;
    height *= 2;
    width *= 2;

    return true;
    //ClearToBlack();
    //return false;
}// Double_Size


///////////////////////////////////////////////////////////////////////////////
//
//      Scale the image dimensions by the given factor.  The given factor is 
//  assumed to be greater than one.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Resize(float scale)
{
    ClearToBlack();
    return false;
}// Resize


//////////////////////////////////////////////////////////////////////////////
//
//      Rotate the image clockwise by the given angle.  Do not resize the 
//  image.  Return success of operation.
//
///////////////////////////////////////////////////////////////////////////////
bool TargaImage::Rotate(float angleDegrees)
{
    angleDegrees *= 3.1415926 / 180.0;      //angle degrees to radians

    // reconstruct with Barlette filter
    float filter_matrix[16] = {
        1, 3, 3, 1,
        3, 9, 9, 3,
        3, 9, 9, 3,
        1, 3, 3, 1,
    };
    filter(filter_matrix, 64, 4);


    // rotate, backward mapping
    // u = x cos() + y sin()
    // v = -x sin() + y cos()

    unsigned char* newdata = new unsigned char[width * height * 4];
    memset(newdata, 0, width * height * 4);

    for (int newH = -height / 2; newH < height / 2; newH++) {
        for (int newW = -width / 2; newW < width / 2; newW++) {
            int w = newW * cos(angleDegrees) + newH * sin(angleDegrees);
            int h = newW * -1 * sin(angleDegrees) + newH * cos(angleDegrees);

            // rgb channel
            for (int i = 0; i < 4; i++) {
                if ((h + height / 2) > 0 && (h + height / 2) < height &&
                    (w + width / 2) > 0 && (w + width / 2) < width) {
                    newdata[(newH + height / 2) * width * 4 + (newW + width / 2) * 4 + i] = data[(h + height / 2) * width * 4 + (w + width / 2) * 4 + i];
                }
            }
        }
    }

    delete[] data;
    data = newdata;

    return true;
    //ClearToBlack();
    //return false;
}// Rotate


//////////////////////////////////////////////////////////////////////////////
//
//      Given a single RGBA pixel return, via the second argument, the RGB
//      equivalent composited with a black background.
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::RGBA_To_RGB(unsigned char *rgba, unsigned char *rgb)
{
    const unsigned char	BACKGROUND[3] = { 0, 0, 0 };

    unsigned char  alpha = rgba[3];

    if (alpha == 0)
    {
        rgb[0] = BACKGROUND[0];
        rgb[1] = BACKGROUND[1];
        rgb[2] = BACKGROUND[2];
    }
    else
    {
	    float	alpha_scale = (float)255 / (float)alpha;
	    int	val;
	    int	i;

	    for (i = 0 ; i < 3 ; i++)
	    {
	        val = (int)floor(rgba[i] * alpha_scale);
	        if (val < 0)
		    rgb[i] = 0;
	        else if (val > 255)
		    rgb[i] = 255;
	        else
		    rgb[i] = val;
	    }
    }
}// RGA_To_RGB


///////////////////////////////////////////////////////////////////////////////
//
//      Copy this into a new image, reversing the rows as it goes. A pointer
//  to the new image is returned.
//
///////////////////////////////////////////////////////////////////////////////
TargaImage* TargaImage::Reverse_Rows(void)
{
    unsigned char   *dest = new unsigned char[width * height * 4];
    TargaImage	    *result;
    int 	        i, j;

    if (! data)
    	return NULL;

    for (i = 0 ; i < height ; i++)
    {
	    int in_offset = (height - i - 1) * width * 4;
	    int out_offset = i * width * 4;

	    for (j = 0 ; j < width ; j++)
        {
	        dest[out_offset + j * 4] = data[in_offset + j * 4];
	        dest[out_offset + j * 4 + 1] = data[in_offset + j * 4 + 1];
	        dest[out_offset + j * 4 + 2] = data[in_offset + j * 4 + 2];
	        dest[out_offset + j * 4 + 3] = data[in_offset + j * 4 + 3];
        }
    }

    result = new TargaImage(width, height, dest);
    delete[] dest;
    return result;
}// Reverse_Rows


///////////////////////////////////////////////////////////////////////////////
//
//      Clear the image to all black.
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::ClearToBlack()
{
    memset(data, 0, width * height * 4);
}// ClearToBlack


///////////////////////////////////////////////////////////////////////////////
//
//      Helper function for the painterly filter; paint a stroke at
// the given location
//
///////////////////////////////////////////////////////////////////////////////
void TargaImage::Paint_Stroke(const Stroke& s) {
   int radius_squared = (int)s.radius * (int)s.radius;
   for (int x_off = -((int)s.radius); x_off <= (int)s.radius; x_off++) {
      for (int y_off = -((int)s.radius); y_off <= (int)s.radius; y_off++) {
         int x_loc = (int)s.x + x_off;
         int y_loc = (int)s.y + y_off;
         // are we inside the circle, and inside the image?
         if ((x_loc >= 0 && x_loc < width && y_loc >= 0 && y_loc < height)) {
            int dist_squared = x_off * x_off + y_off * y_off;
            if (dist_squared <= radius_squared) {
               data[(y_loc * width + x_loc) * 4 + 0] = s.r;
               data[(y_loc * width + x_loc) * 4 + 1] = s.g;
               data[(y_loc * width + x_loc) * 4 + 2] = s.b;
               data[(y_loc * width + x_loc) * 4 + 3] = s.a;
            } else if (dist_squared == radius_squared + 1) {
               data[(y_loc * width + x_loc) * 4 + 0] = 
                  (data[(y_loc * width + x_loc) * 4 + 0] + s.r) / 2;
               data[(y_loc * width + x_loc) * 4 + 1] = 
                  (data[(y_loc * width + x_loc) * 4 + 1] + s.g) / 2;
               data[(y_loc * width + x_loc) * 4 + 2] = 
                  (data[(y_loc * width + x_loc) * 4 + 2] + s.b) / 2;
               data[(y_loc * width + x_loc) * 4 + 3] = 
                  (data[(y_loc * width + x_loc) * 4 + 3] + s.a) / 2;
            }
         }
      }
   }
}


///////////////////////////////////////////////////////////////////////////////
//
//      Build a Stroke
//
///////////////////////////////////////////////////////////////////////////////
Stroke::Stroke() {}

///////////////////////////////////////////////////////////////////////////////
//
//      Build a Stroke
//
///////////////////////////////////////////////////////////////////////////////
Stroke::Stroke(unsigned int iradius, unsigned int ix, unsigned int iy,
               unsigned char ir, unsigned char ig, unsigned char ib, unsigned char ia) :
   radius(iradius),x(ix),y(iy),r(ir),g(ig),b(ib),a(ia)
{
}

