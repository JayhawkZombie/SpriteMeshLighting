// SFUI.cpp : Defines the entry point for the console application.
//

#ifdef _WIN32 || defined(_WIN64)
#include "stdafx.h"
#else

#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <iostream>
#include <opencv2/core/hal/interface.h>

#include <SFML/Graphics.hpp>

#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <map>
#include <iostream>
#include <functional>
#include <chrono>

#endif

/*
 * Contour generation code
 * 
 * Generate a contour of a sprite from a given binary file and 
 * save it to a file called "img.contour"
 * 
 * The contour is just a file of integer values
 * Every 2 values constitutes a point along the contour
 **/

/*
 *  From https://docs.opencv.org/3.3.1/df/d0d/tutorial_find_contours.html
 **/

//#include "opencv2/imgcodecs.hpp"
//#include "opencv2/highgui.hpp"
//#include "opencv2/imgproc.hpp"
//#include <iostream>
//#include <opencv2/core/hal/interface.h>
//
//
//using namespace cv;
//using namespace std;
//Mat src; Mat src_gray;
//int thresh = 100;
//int max_thresh = 255;
//RNG rng(12345);
//void thresh_callback(int, void*);
//int main(int argc, char** argv)
//{
//  String imageName("bwbinary.png"); // by default
//  if (argc > 1)
//  {
//    imageName = argv[1];
//  }
//  src = imread(imageName, IMREAD_COLOR);
//  if (src.empty())
//  {
//    cerr << "No image supplied ..." << endl;
//    return -1;
//  }
//  cvtColor(src, src_gray, COLOR_BGR2GRAY);
//  blur(src_gray, src_gray, Size(1, 1));
//  const char* source_window = "Source";
//  namedWindow(source_window, WINDOW_AUTOSIZE);
//  imshow(source_window, src);
//  createTrackbar(" Canny thresh:", "Source", &thresh, max_thresh, thresh_callback);
//  thresh_callback(0, 0);
//  waitKey(0);
//  return( 0 );
//}
//void thresh_callback(int, void*)
//{
//  Mat canny_output;
//  vector<vector<Point> > contours;
//  vector<Vec4i> hierarchy;
//  Canny(src_gray, canny_output, thresh, thresh * 2, 3);
//  findContours(canny_output, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_TC89_L1, Point(0, 0));
//  Mat drawing = Mat::zeros(canny_output.size(), CV_8UC3);
//
//  static bool once = false;
//  if (!once) {
//
//    once = true;
//
//    std::ofstream contFile("img.contour");
//    for (const auto & ptvec : contours) {
//
//      for (const auto & pt : ptvec) {
//        contFile << pt.x << ' ' << pt.y << ' ';
//      }
//
//      contFile << "\n";
//    }
//
//    contFile.close();
//  }
//
//  for (size_t i = 0; i < contours.size(); i++)
//  {
//    Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
//    drawContours(drawing, contours, ( int )i, color, 1, 8, hierarchy, 0, Point());
//  }
//  namedWindow("Contours", WINDOW_AUTOSIZE);
//  imshow("Contours", drawing);
//}

using Edge = std::pair<sf::Vector2f, sf::Vector2f>;
#include <cmath>

int main()
{

  /* Stuff related to the window */
  sf::RenderWindow Window(sf::VideoMode(800, 800), "Test", sf::Style::Default);
  sf::Event event;
  bool Closed = false;
  
  /* Light source & attenuation */
  float atten = 400.f;
  sf::Vector2f LightPos(0.f, 0.f);

  /* texture of the original sprite and render texture to render
   * shadow map into 
   * */
  sf::Texture zombie;
  sf::Sprite zombieGirl;
  sf::RenderTexture shadowMap;

  zombie.loadFromFile("zombie.png");
  zombieGirl.setTexture(zombie);
  shadowMap.create(800, 800);

  /* Generate the edges - every pair of 2 points in the contour */
  sf::Vector2i pt1, pt2;
  std::vector<sf::Vector2f> points;
  std::vector<Edge> edges;

  /* File containing the contour data */
  std::ifstream cont("img.contour");
  while (cont >> pt1.x >> pt1.y) { points.push_back(static_cast< sf::Vector2f >( pt1 )); }
  cont.close();

  int idx1, idx2;

  for (int i = 0; i < points.size(); i++) {
    idx1 = i;
    idx2 = ( i == points.size() - 1 ? 0 : i + 1 );

    edges.push_back(std::make_pair(points[idx1], points[idx2]));
  }

  /* VertexArray - render shadow volumes as triangular regions */
  sf::VertexArray v(sf::Triangles);

  sf::Vector2f dir1, dir2;

  auto normalize = [](sf::Vector2f &vec)
  {
    auto mag = std::hypot(vec.x, vec.y);
    return vec / mag;
  };

  sf::Vector2f back1, back2;
  sf::Vertex vert1, vert2, vert3, vert4;
  vert1.color = vert2.color = vert3.color = vert4.color = sf::Color(122, 122, 122);

  /* Generate shadow volumes by extruding shadow volumes behind the edges */
  for (const auto & edge : edges) {

    dir1 = edge.first - LightPos;
    normalize(dir1) - LightPos;

    dir2 = edge.second;
    normalize(dir2);

    back1 = edge.first + atten * dir1;
    back2 = edge.second + atten * dir2;

    vert1.position = edge.first;
    vert2.position = edge.second;
    vert3.position = back1;
    vert4.position = back2;

    v.append(vert1); v.append(vert2); v.append(vert3);
    v.append(vert2); v.append(vert3); v.append(vert4);
  }

  sf::RenderStates state;
  state.blendMode = sf::BlendAlpha;

  /* Render shadow volumes - only needs to be done once
   * if the object and light never move
   * */
  shadowMap.clear(sf::Color::Transparent);
  shadowMap.draw(v, state);
  shadowMap.display();

  /* Rect to use to render the shadow volume onto the scene with */
  sf::RectangleShape rect;
  rect.setTexture(&shadowMap.getTexture());
  rect.setSize(sf::Vector2f(800.f, 800.f));
  rect.setFillColor(sf::Color(255, 255, 255, 122));

  /* Boring SFML loop */
  while (!Closed) {

    while (Window.pollEvent(event)) {
      Closed = ( event.type == sf::Event::Closed );
    }

    Window.clear(sf::Color::White);
    Window.draw(zombieGirl);
    Window.draw(rect);
    Window.display();
  }

  Window.close();

  return 0;
}

