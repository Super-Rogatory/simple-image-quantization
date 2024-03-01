#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>

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

/**
 * Class that implements wxFrame.
 * This frame serves as the top level window for the program
 */
class MyFrame : public wxFrame {
 public:
  MyFrame(const wxString &title, string imagePath, int mode, int max_unique_colors);
  void Quant();
  void performUniformQuantization(int max_unique_colors);
  void uniformInitalization(int partitions); 
  void uniformlyPopulateBuckets(int partitions); 
  std::vector<unsigned char> uniformNewRGB(unsigned char r, unsigned char g, unsigned char b); 

 private:
  void OnPaint(wxPaintEvent &event);
  wxImage inImage;
  wxScrolledWindow *scrolledWindow;
  unsigned char* inData;
  int width;
  int height;
  int mode;
  int max_unique_colors;
  std::vector<std::pair<int, int>> buckets;
  std::vector<int> midpoints;
};

/** Utility function to read image data */
unsigned char *readImageData(string imagePath, int width, int height);

/** Definitions */

std::vector<unsigned char> MyFrame::uniformNewRGB(unsigned char r, unsigned char g, unsigned char b) {
  unsigned char n_r = 0, n_g = 0, n_b = 0;
  std::size_t count = 0;
  for(int i = 0; i < buckets.size(); ++i) {
    if(r >= buckets[i].first && r <= buckets[i].second) {
      n_r = midpoints[i];
      ++count;
    }
    if(g >= buckets[i].first && g <= buckets[i].second) {
      n_g = midpoints[i];
      ++count;
    }
    if(b >= buckets[i].first && b <= buckets[i].second) {
      n_b = midpoints[i];      
      ++count;
    }

    if(count == 3)
      break;    
  }
  return { n_r, n_g, n_b };
}

// sliding window (in a way) - oh wow, I guess leetcode isn't uselss afterall. . .
void MyFrame::uniformlyPopulateBuckets(int partitions) {
  // from 0 to 255.
  // double loop 
  const int MAX = 256; // for any 8 bit pixel, 256 unique values per channel
  const int RANGE_SIZE = 256 / partitions;
  int i = 0, j = 0;
  while(i < MAX) {
    j = i + RANGE_SIZE - 1;
    buckets.push_back({ i, max(j, MAX - 1) });
    i = j + 1;
  }
}



// initializing buckets and midpoints for uniform quant
void MyFrame::uniformInitalization(int partitions) {
    const int MAX = 256; // for any 8-bit pixel, 256 unique values per channel
    const int base_range_size = MAX / partitions;
    int leftovers = MAX % partitions; // calculate leftover values to distribute
    int i = 0, j = 0, range_size = 0;

    for (int part = 0; part < partitions; ++part) {
        // each partition gets plus one extra value if we have any left to distribute, ensuring partition sizes kept
        range_size = base_range_size + (leftovers > 0 ? 1 : 0);
        j = i + range_size - 1;
        buckets.push_back({i, j});
        midpoints.push_back(round((j + i) / 2.0f));
        i = j + 1;
        --leftovers;
    }
}

void MyFrame::performUniformQuantization(int max_unique_colors) {
  // could I use instead int* buckets? so each element points to an int array? Are there speed improvements using a raw array?
  uniformInitalization(std::pow(max_unique_colors, 1/3.0)); // root3(B)
  // now we have bucket and midpoint information, we can perform quants on each color. let's gooooo.
  std::vector<unsigned char> newRGB;
  int index;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
        index = (y * width + x) * 3;
        newRGB = uniformNewRGB(inData[index], inData[index + 1], inData[index + 2]);
        inData[index] = newRGB[0];
        inData[index + 1] = newRGB[1];
        inData[index + 2] = newRGB[2];
    }
  }
}

void MyFrame::Quant() {
  // using this so it's clear where the data is coming from
  switch(this->mode) {
    case 1:
      performUniformQuantization(this->max_unique_colors);
      break;
    case 2:
      // performNonUniformQuantization(this->inData, this->max_unique_colors);
      break;
    default:
      break;
  }
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
  if (wxApp::argc != 4) {
      cerr << "The executable should be invoked with exactly one filepath "
              "argument. Example: ./MyImageApplication '../../Lena_512_512.rgb' "
              "followed by the mode (uniform or non-uniform quantization) and the number of unique colors in the output."
          << endl;
    exit(1);
  }
  cout << "First argument: " << wxApp::argv[0] << endl;
  cout << "Second argument: " << wxApp::argv[1] << endl;
  cout << "Q: " << wxApp::argv[2] << endl;
  cout << "B: " << wxApp::argv[3] << endl;

  string imagePath = wxApp::argv[1].ToStdString();
  int mode = std::stoi(wxApp::argv[2].ToStdString());
  int max_unique_colors = std::stoi(wxApp::argv[3].ToStdString());

  MyFrame *frame = new MyFrame("Image Display", imagePath, mode, max_unique_colors);
  frame->Quant();
  frame->Show(true);

  // return true to continue, false to exit the application
  return true;
}

/**
 * Constructor for the MyFrame class.
 * Here we read the pixel data from the file and set up the scrollable window.
 */
MyFrame::MyFrame(const wxString &title, string imagePath, int mode, int max_unique_colors)
    : wxFrame(NULL, wxID_ANY, title), mode(mode), max_unique_colors(max_unique_colors) {

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
void MyFrame::OnPaint(wxPaintEvent &event) {
  wxBufferedPaintDC dc(scrolledWindow);
  scrolledWindow->DoPrepareDC(dc);

  wxBitmap inImageBitmap = wxBitmap(inImage);
  dc.DrawBitmap(inImageBitmap, 0, 0, false);
}

/** Utility function to read image data */
unsigned char *readImageData(string imagePath, int width, int height) {

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
  unsigned char *inData =
      (unsigned char *)malloc(width * height * 3 * sizeof(unsigned char));
      
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