// 3D Konsolen-Labyrinth in C++ (Raycasting-Prinzip, wie im alten Wolfenstein 3D)
//
// Kompilieren (Linux/Mac):
//   g++ -O2 -o labyrinth main.cpp
//   ./labyrinth
//
// Kompilieren (Windows, z.B. mit MinGW):
//   g++ -O2 -o labyrinth.exe main.cpp
//   labyrinth.exe
//
// Steuerung:
//   W / S = vorwaerts / rueckwaerts
//   A / D = nach links / rechts drehen
//   Q     = Spiel beenden
//
// Hinweis: Konsolenfenster vor dem Start etwas vergroessern und eine
// kleine Schriftgroesse waehlen, dann sieht die 3D-Ansicht schoener aus.

#include <iostream>
#include <string>
#include <cmath>
#include <chrono>
#include <thread>

#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
#else
    #include <termios.h>
    #include <unistd.h>
    #include <fcntl.h>
#endif

// ---------- Plattformabhaengige Tastatureingabe ----------
#ifndef _WIN32
bool tasteVerfuegbar()
{
    termios alt, neu;
    tcgetattr(STDIN_FILENO, &alt);
    neu = alt;
    neu.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &neu);

    int alteFlags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, alteFlags | O_NONBLOCK);

    int zeichen = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &alt);
    fcntl(STDIN_FILENO, F_SETFL, alteFlags);

    if (zeichen != EOF)
    {
        ungetc(zeichen, stdin);
        return true;
    }
    return false;
}

char letzteTasteHolen()
{
    return getchar();
}
#endif

char TasteLesenFallsVorhanden()
{
#ifdef _WIN32
    if (_kbhit())
    {
        return _getch();
    }
    return 0;
#else
    if (tasteVerfuegbar())
    {
        return letzteTasteHolen();
    }
    return 0;
#endif
}

void BildschirmLoeschen()
{
#ifdef _WIN32
    system("cls");
#else
    std::cout << "\033[2J\033[H"; // ANSI: loescht Bildschirm, Cursor an Position 0,0
#endif
}

// ---------- Spielkarte ----------
// 1 = Wand, 0 = freier Weg
int karte[11][16] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1},
    {1,0,1,1,1,0,1,1,1,0,1,0,1,1,0,1},
    {1,0,1,0,0,0,0,0,1,0,0,0,1,0,0,1},
    {1,0,1,0,1,1,1,0,1,1,1,0,1,0,1,1},
    {1,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1},
    {1,0,1,0,1,0,1,1,1,0,1,1,1,1,0,1},
    {1,0,1,0,0,0,0,0,1,0,0,0,0,1,0,1},
    {1,0,1,1,1,1,1,0,1,0,1,1,0,1,0,1},
    {1,0,0,0,0,0,1,0,0,0,1,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
};

const int KARTE_HOEHE = 11;
const int KARTE_BREITE = 16;

const int BILDSCHIRM_BREITE = 100;
const int BILDSCHIRM_HOEHE = 35;
const double SICHTWEITE = 16.0;
const double SICHTFELD = M_PI / 3.0; // 60 Grad

double spielerX = 2.5;
double spielerY = 2.5;
double blickwinkel = 0.0;

void ZeichneBild()
{
    // Fuer jede Bildschirmspalte einen Strahl schiessen und die Wandhoehe berechnen
    static char bild[BILDSCHIRM_HOEHE][BILDSCHIRM_BREITE + 1];

    for (int spalte = 0; spalte < BILDSCHIRM_BREITE; spalte++)
    {
        double strahlWinkel = (blickwinkel - SICHTFELD / 2.0)
                             + ((double)spalte / BILDSCHIRM_BREITE) * SICHTFELD;

        double entfernung = 0.0;
        bool wandGetroffen = false;
        double testX = 0, testY = 0;

        while (!wandGetroffen && entfernung < SICHTWEITE)
        {
            entfernung += 0.1;
            testX = spielerX + std::cos(strahlWinkel) * entfernung;
            testY = spielerY + std::sin(strahlWinkel) * entfernung;

            int kartenX = (int)testX;
            int kartenY = (int)testY;

            if (kartenX < 0 || kartenX >= KARTE_BREITE || kartenY < 0 || kartenY >= KARTE_HOEHE)
            {
                wandGetroffen = true;
                entfernung = SICHTWEITE;
            }
            else if (karte[kartenY][kartenX] == 1)
            {
                wandGetroffen = true;
            }
        }

        int wandHoehe = (int)(BILDSCHIRM_HOEHE / entfernung);
        int mitte = BILDSCHIRM_HOEHE / 2;
        int wandOben = std::max(0, mitte - wandHoehe / 2);
        int wandUnten = std::min(BILDSCHIRM_HOEHE - 1, mitte + wandHoehe / 2);

        char wandZeichen = entfernung < SICHTWEITE / 4.0   ? '#'
                          : entfernung < SICHTWEITE / 2.0   ? '+'
                          : entfernung < SICHTWEITE * 0.75  ? '-'
                                                             : '.';

        for (int zeile = 0; zeile < BILDSCHIRM_HOEHE; zeile++)
        {
            if (zeile < wandOben)       bild[zeile][spalte] = ' ';   // Decke
            else if (zeile <= wandUnten) bild[zeile][spalte] = wandZeichen; // Wand
            else                         bild[zeile][spalte] = '_';  // Boden
        }
    }

    BildschirmLoeschen();
    std::cout << "=== C++ 3D KONSOLEN-LABYRINTH ===  (W/S = laufen, A/D = drehen, Q = beenden)\n";
    for (int zeile = 0; zeile < BILDSCHIRM_HOEHE; zeile++)
    {
        bild[zeile][BILDSCHIRM_BREITE] = '\0';
        std::cout << bild[zeile] << "\n";
    }
}

int main()
{
    bool laeuft = true;

    while (laeuft)
    {
        char taste = TasteLesenFallsVorhanden();
        double bewegungsSchritt = 0.25;
        double drehSchritt = 0.15;

        double neuX = spielerX;
        double neuY = spielerY;

        switch (taste)
        {
            case 'w': case 'W':
                neuX += std::cos(blickwinkel) * bewegungsSchritt;
                neuY += std::sin(blickwinkel) * bewegungsSchritt;
                break;
            case 's': case 'S':
                neuX -= std::cos(blickwinkel) * bewegungsSchritt;
                neuY -= std::sin(blickwinkel) * bewegungsSchritt;
                break;
            case 'a': case 'A':
                blickwinkel -= drehSchritt;
                break;
            case 'd': case 'D':
                blickwinkel += drehSchritt;
                break;
            case 'q': case 'Q':
                laeuft = false;
                break;
        }

        if (karte[(int)neuY][(int)neuX] == 0)
        {
            spielerX = neuX;
            spielerY = neuY;
        }

        ZeichneBild();
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    BildschirmLoeschen();
    std::cout << "Spiel beendet. Bis zum naechsten Mal!\n";
    return 0;
}
