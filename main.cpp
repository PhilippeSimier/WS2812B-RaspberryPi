/* 
 * File:   main.cpp
 * Author: philippe SIMIER Lycée touchard Le Mans
 *
 * Created on 12 octobre 2024, 17:07
 */

#include <iostream>
#include "ws2812b.h"


using namespace std;

int main(int argc, char** argv) {

    try {

        ws2812b ledRGB(1); //1 pixel LED RGB

        while (1) {
            //RGB Blink.
            ledRGB.setPixelColor(0, 5, 0, 0); // rouge
            ledRGB.show();
            cout << "rouge" << endl;
            sleep(3);

            ledRGB.setPixelColor(0, 2, 2, 0); // jaune
            ledRGB.show();
            cout << "jaune" << endl;
            sleep(3);

            ledRGB.setPixelColor(0, 0, 5, 0); // vert
            ledRGB.show();
            cout << "vert" << endl;
            sleep(3);

            ledRGB.setPixelColor(0, 0, 0, 5); // Bleu
            ledRGB.show();
            cout << "bleu" << endl;
            sleep(3);

        }
    } catch (const std::runtime_error &e) {

        cout << "Exception caught: " << e.what() << endl;
    }

    return 0;
}

