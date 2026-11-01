#ifndef ZENGINE_H
#define ZENGINE_H

struct HWND__;

class zCEngine {
public:

    zCEngine();
    virtual ~zCEngine();
    static int Init(HWND__** window);
    static int Shutdown();
};

extern zCEngine* zengine;

class zCConsole;
class zCParser;

extern zCConsole* edit_con;
extern zCConsole* game_cam_con;
extern zCParser* parserSoundFX;
extern zCParser* parserParticleFX;
extern zCParser* parserVisualFX;
extern zCParser* parserCamera;
extern zCParser* parserMenu;
extern zCParser* parserMusic;

#endif
