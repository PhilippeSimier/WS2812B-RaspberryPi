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
    
    Color couleur[8] = { RED, GREEN, BLUE, WHITE, BLACK, YELLOW, CYAN, MAGENTA };
    
    try {

        WS2812b ledRGB(1); //1 pixel LED RGB
        
        int i = 0;
        while (1) {
            //RGB Blink.
            ledRGB.setPixelColor(0, couleur[i], 0.1); 
            ledRGB.show();
            sleep(3);
            i++;
            if (i == 8) i=0;
        }
    } catch (const std::runtime_error &e) {

        cout << "Exception caught: " << e.what() << endl;
    }

    return 0;
}

