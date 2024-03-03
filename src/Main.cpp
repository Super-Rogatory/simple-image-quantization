#include <wx/dcbuffer.h>
#include <wx/wx.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

/**
 * Display an image using WxWidgets.
 * https://www.wxwidgets.org/
 */

/** Declarations*/

/**
 * Class that implements wxApp
 */
class MyApp : public wxApp {
  public:
  bool OnInit() override;
};

/* OSSTTREEAAAMMMM */
// Overload the stream insertion operator for std::pair<int, int>
std::ostream& operator<<(std::ostream& os, const std::pair<int, int>& p) {
  os << "[" << p.first << ", " << p.second << "]";
  return os;
}

// Overload the stream insertion operator for std::vector<std::pair<int, int>>
std::ostream& operator<<(std::ostream& os, const std::vector<std::pair<int, int>>& v) {
  os << "{ ";
  for (const auto& pair : v) { os << pair << " "; }
  os << "}";
  return os;
}

// Overload the stream insertion operator for std::vector<std::pair<int, int>>
std::ostream& operator<<(std::ostream& os, const std::vector<int>& v) {
  os << "{ ";
  for (const auto& elem : v) { os << elem << " "; }
  os << "}";
  return os;
}
/* OSSTTREEAAAMMMM */

/**
 * Class that implements wxFrame.
 * This frame serves as the top level window for the program
 */
typedef unsigned char pixel;
class MyFrame : public wxFrame {
  public:
  MyFrame(const wxString& title, string imagePath, int mode, long long max_unique_colors, bool debugMode);
  void Quant();
  void performUniformQuantization();
  void uniformInitalization();
  std::vector<pixel> uniformNewRGB(pixel r, pixel g, pixel b);
  void performNonUniformQuantization();
  void nonUniformInitialization();
  std::vector<pixel> nonUniformNewRGB(pixel r, pixel g, pixel b);
  void displayError();
  bool getViewMode() const { return this->viewImage; };

  private:
  void OnPaint(wxPaintEvent& event);
  wxImage inImage;
  wxScrolledWindow* scrolledWindow;
  pixel* inData;
  int width;
  int height;
  int mode;
  bool viewImage;
  int partitions;
  long long max_unique_colors;
  std::vector<std::pair<pixel, pixel>> buckets;  // ranges
  std::vector<pixel> midpoints;
  std::vector<std::pair<pixel, pixel>> red_buckets;    // ranges
  std::vector<std::pair<pixel, pixel>> green_buckets;  // ranges
  std::vector<std::pair<pixel, pixel>> blue_buckets;   // ranges
  std::vector<std::vector<pixel>> reds;
  std::vector<std::vector<pixel>> greens;
  std::vector<std::vector<pixel>> blues;
  std::vector<pixel> red_averages;
  std::vector<pixel> blue_averages;
  std::vector<pixel> green_averages;
  std::vector<std::vector<float>> uniformErrorsPerChannel;
  std::vector<std::vector<float>> nonUniformErrorsPerChannel;
  std::vector<std::pair<int, float>> uniformErrors;
  std::vector<std::pair<int, float>> nonUniformErrors;
  long sumOfAbsoluteRedError;
  long sumOfAbsoluteGreenError;
  long sumOfAbsoluteBlueError;
};

/** Utility function to read image data */
pixel* readImageData(string imagePath, int width, int height);

/** Definitions */

std::vector<pixel> MyFrame::uniformNewRGB(pixel r, pixel g, pixel b) {
  pixel n_r = 0, n_g = 0, n_b = 0;
  std::size_t count = 0;
  for (int i = 0; i < buckets.size(); ++i) {
    if (r >= buckets[i].first && r <= buckets[i].second) {
      n_r = midpoints[i];
      ++count;
    }
    if (g >= buckets[i].first && g <= buckets[i].second) {
      n_g = midpoints[i];
      ++count;
    }
    if (b >= buckets[i].first && b <= buckets[i].second) {
      n_b = midpoints[i];
      ++count;
    }

    if (count == 3) break;
  }
  return {n_r, n_g, n_b};
}

// initializing buckets and midpoints for uniform quant
void MyFrame::uniformInitalization() {
  // partitions - the number of buckets we should have for each color channel
  const int MAX = 256;  // for any 8-bit pixel, 256 unique values per channel
  const int base_range_size = MAX / partitions;
  int leftovers = MAX % partitions;  // calculate leftover values to distribute
  int i = 0, j = 0, range_size = 0;

  for (int part = 0; part < partitions; ++part) {
    // each partition gets plus one extra value if we have any left to distribute, ensuring partition sizes kept
    range_size = base_range_size + (leftovers > 0 ? 1 : 0);
    j = i + range_size - 1;
    buckets.push_back({i, j});
    midpoints.push_back(round((j + i) / 2.0f));
    i = j + 1;
    if (leftovers > 0) --leftovers;
  }
}

void MyFrame::performUniformQuantization() {
  // could I use instead int* buckets? so each element points to an int array? Are there speed improvements using a raw array?
  uniformInitalization();  // root3(B)
  // now we have bucket and midpoint information, we can perform quants on each color. let's gooooo.
  std::vector<pixel> newRGB;
  int index;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      index = (y * width + x) * 3;
      newRGB = uniformNewRGB(inData[index], inData[index + 1], inData[index + 2]);
      sumOfAbsoluteRedError += abs(inData[index] - newRGB[0]);
      sumOfAbsoluteGreenError += abs(inData[index + 1] - newRGB[1]);
      sumOfAbsoluteBlueError += abs(inData[index + 2] - newRGB[2]);      
      inData[index] = newRGB[0];
      inData[index + 1] = newRGB[1];
      inData[index + 2] = newRGB[2];
    }
  }
}

std::vector<pixel> MyFrame::nonUniformNewRGB(pixel r, pixel g, pixel b) {
  pixel n_r = 0, n_g = 0, n_b = 0;
  std::size_t count = 0;
  bool is_last = false;
  // all buckets should be of the same size, ensuring exclusive boundary conditions unless it is the last bucket range.
  // functions similarly to uniform quantization.
  for (int i = 0; i < red_buckets.size(); ++i) {
    is_last = (i == red_buckets.size() - 1);
    if ((r >= red_buckets[i].first && r < red_buckets[i].second) || (is_last && r == red_buckets[i].second)) {
      n_r = red_averages[i];
      ++count;
    }
    if ((g >= green_buckets[i].first && g < green_buckets[i].second) || (is_last && g == green_buckets[i].second)) {
      n_g = green_averages[i];
      ++count;
    }
    if ((b >= blue_buckets[i].first && b < blue_buckets[i].second) || (is_last && b == blue_buckets[i].second)) {
      n_b = blue_averages[i];
      ++count;
    }
    if (count == 3) break;
  }
  return {n_r, n_g, n_b};
}

void MyFrame::nonUniformInitialization() {
  // partitions - the number of buckets we should have for each color channel
  const int TOTAL_PIXEL_COUNT = width * height;                // we get the total pixel count
  const int base_range_size = TOTAL_PIXEL_COUNT / partitions;  // each partition should have a uniform # of pixels!
  int leftovers = TOTAL_PIXEL_COUNT % partitions;              // calculate leftover values to distribute
  std::vector<pixel> reds_values;
  std::vector<pixel> greens_values;
  std::vector<pixel> blues_values;
  int index = 0;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      index = (y * width + x) * 3;
      reds_values.push_back(inData[index]);
      greens_values.push_back(inData[index + 1]);
      blues_values.push_back(inData[index + 2]);
    }
  }

  // let us examine the frequencys of pixel values
  std::sort(reds_values.begin(), reds_values.end());
  std::sort(greens_values.begin(), greens_values.end());
  std::sort(blues_values.begin(), blues_values.end());
  // now let us partition the proper number of pixels
  // ie.) [1,1,1,1,1,2,3,3,4,4,5,5,5,5,5,5,6,6,7,8,9] - reds (approx 21 vals), 4 partitions leftover = 1
  std::vector<pixel> red_range, green_range, blue_range;
  int range_index = 0, range_size = 0;
  for (int part = 0; part < partitions; ++part) {
    range_size = base_range_size + (leftovers > 0 ? 1 : 0);
    // yields range subvector
    red_range = std::vector<pixel>(reds_values.begin() + range_index,
                                           reds_values.begin() + min(range_index + range_size, static_cast<int>(reds_values.size())));
    green_range = std::vector<pixel>(greens_values.begin() + range_index,
                                             greens_values.begin() + min(range_index + range_size, static_cast<int>(greens_values.size())));
    blue_range = std::vector<pixel>(blues_values.begin() + range_index,
                                            blues_values.begin() + min(range_index + range_size, static_cast<int>(blues_values.size())));
    // pushes low and highest value in range (since sorted)
    red_buckets.push_back({red_range.front(), red_range.back()});
    green_buckets.push_back({green_range.front(), green_range.back()});
    blue_buckets.push_back({blue_range.front(), blue_range.back()});
    // places average of each range
    red_averages.push_back(static_cast<pixel>(
        std::round(std::accumulate(red_range.begin(), red_range.end(), 0L) / static_cast<float>(red_range.size()))));
    green_averages.push_back(static_cast<pixel>(
        std::round(std::accumulate(green_range.begin(), green_range.end(), 0L) / static_cast<float>(green_range.size()))));
    blue_averages.push_back(static_cast<pixel>(
        std::round(std::accumulate(blue_range.begin(), blue_range.end(), 0L) / static_cast<float>(blue_range.size()))));

    range_index += range_size;
    if (leftovers > 0) --leftovers;
  }
}

void MyFrame::performNonUniformQuantization() {
  // pseudocode
  // 8-bit image: can have 256 unique colors each channel
  // now I have the frequencies for each color seperated into their respective buckets, along with their averages
  nonUniformInitialization();
  std::vector<pixel> newRGB;
  int index;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      index = (y * width + x) * 3;
      newRGB = nonUniformNewRGB(inData[index], inData[index + 1], inData[index + 2]);
      // do it here
      sumOfAbsoluteRedError += abs(inData[index] - newRGB[0]);
      sumOfAbsoluteGreenError += abs(inData[index + 1] - newRGB[1]);
      sumOfAbsoluteBlueError += abs(inData[index + 2] - newRGB[2]);
      inData[index] = newRGB[0];
      inData[index + 1] = newRGB[1];
      inData[index + 2] = newRGB[2];
    }
  }
}

void MyFrame::Quant() {
  // using this so it's clear where the data is coming from
  partitions = static_cast<int>(std::round(std::pow(max_unique_colors, 1 / 3.0)));
  switch (this->mode) {
    case 1:
      performUniformQuantization();
      break;
    case 2:
      performNonUniformQuantization();
      break;
    default:
      break;
  }
  // we want the error with respect to the bucket number we test in the input to print in the console
  // for all color counts 3root(B)'s from 2 to 256, we need to find the errors and plot them (in report)
  // do the report with three images
}

void MyFrame::displayError() {
  std::cout << "Total Error Sum: " << sumOfAbsoluteRedError + sumOfAbsoluteGreenError + sumOfAbsoluteBlueError << std::endl;
}


/**
 * Init method for the app.
 * Here we process the command line arguments and
 * instantiate the frame.
 */
bool MyApp::OnInit() {
  wxInitAllImageHandlers();

  // deal with command line arguments here
  cout << "Number of command line arguments: " << wxApp::argc << endl;
  bool viewImage = true;
  if (wxApp::argc != 4 && wxApp::argc != 5) {
    cerr << "The executable should be invoked with exactly one filepath "
            "argument. Example: ./MyImageApplication '../../Lena_512_512.rgb' "
            "followed by the mode (uniform or non-uniform quantization) and the number of unique colors in the output."
         << endl;
    exit(1);
  } else if(wxApp::argc == 5) {
    std::string debugArgument = wxApp::argv[4].ToStdString();
    if(debugArgument == "n" || debugArgument == "N")
      viewImage = false;
  }
  cout << "First argument: " << wxApp::argv[0] << endl;
  cout << "Second argument: " << wxApp::argv[1] << endl;
  cout << "Q: " << wxApp::argv[2] << endl;
  cout << "B: " << wxApp::argv[3] << endl;
  if(!viewImage)
    cout << "View Image Mode Off" << endl;
  string imagePath = wxApp::argv[1].ToStdString();
  int mode = std::stoi(wxApp::argv[2].ToStdString());
  long long max_unique_colors =
      std::stoll(wxApp::argv[3].ToStdString());
  MyFrame* frame = new MyFrame("Image Display", imagePath, mode, max_unique_colors, viewImage);
  frame->Quant();
  if(frame->getViewMode()) 
    frame->Show(true);
  // calculate the total errors for each bucket size
  frame->displayError();
  // return true to continue, false to exit the application
  return true;
}

/**
 * Constructor for the MyFrame class.
 * Here we read the pixel data from the file and set up the scrollable window.
 */
MyFrame::MyFrame(const wxString& title, string imagePath, int mode, long long max_unique_colors, bool viewImage)
    : wxFrame(NULL, wxID_ANY, title), mode(mode), max_unique_colors(max_unique_colors), viewImage(viewImage),
      sumOfAbsoluteRedError(0), sumOfAbsoluteGreenError(0), sumOfAbsoluteBlueError(0)  {
  // Modify the height and width values here to read and display an image with
  // different dimensions.
  width = 512;
  height = 512;

  // yields rgb formatted data
  inData = readImageData(imagePath, width, height);

  // the last argument is static_data, if it is false, after this call the
  // pointer to the data is owned by the wxImage object, which will be
  // responsible for deleting it. So this means that you should not delete the
  // data yourself.
  inImage.SetData(inData, width, height, false);

  // Set up the scrolled window as a child of this frame
  scrolledWindow = new wxScrolledWindow(this, wxID_ANY);
  scrolledWindow->SetScrollbars(10, 10, width, height);
  scrolledWindow->SetVirtualSize(width, height);

  // Bind the paint event to the OnPaint function of the scrolled window
  scrolledWindow->Bind(wxEVT_PAINT, &MyFrame::OnPaint, this);

  // Set the frame size
  SetClientSize(width, height);

  // Set the frame background color
  SetBackgroundColour(*wxBLACK);
}

/**
 * The OnPaint handler that paints the UI.
 * Here we paint the image pixels into the scrollable window.
 */
void MyFrame::OnPaint(wxPaintEvent& event) {
  wxBufferedPaintDC dc(scrolledWindow);
  scrolledWindow->DoPrepareDC(dc);

  wxBitmap inImageBitmap = wxBitmap(inImage);
  dc.DrawBitmap(inImageBitmap, 0, 0, false);
}

/** Utility function to read image data */
pixel* readImageData(string imagePath, int width, int height) {
  // Open the file in binary mode
  ifstream inputFile(imagePath, ios::binary);

  if (!inputFile.is_open()) {
    cerr << "Error Opening File for Reading" << endl;
    exit(1);
  }

  // Create and populate RGB buffers
  vector<char> Rbuf(width * height);
  vector<char> Gbuf(width * height);
  vector<char> Bbuf(width * height);

  /**
   * The input RGB file is formatted as RRRR.....GGGG....BBBB.
   * i.e the R values of all the pixels followed by the G values
   * of all the pixels followed by the B values of all pixels.
   * Hence we read the data in that order.
   */

  inputFile.read(Rbuf.data(), width * height);
  inputFile.read(Gbuf.data(), width * height);
  inputFile.read(Bbuf.data(), width * height);

  inputFile.close();

  /**
   * Allocate a buffer to store the pixel values
   * The data must be allocated with malloc(), NOT with operator new. wxWidgets
   * library requires this.
   */
  pixel* inData = (pixel*)malloc(width * height * 3 * sizeof(pixel));

  for (int i = 0; i < height * width; i++) {
    // We populate RGB values of each pixel in that order
    // RGB.RGB.RGB and so on for all pixels
    inData[3 * i] = Rbuf[i];
    inData[3 * i + 1] = Gbuf[i];
    inData[3 * i + 2] = Bbuf[i];
  }

  return inData;
}

wxIMPLEMENT_APP(MyApp);