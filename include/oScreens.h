#ifndef OSCREENS_H
#define OSCREENS_H

class oCLogScreen {
public:

    virtual ~oCLogScreen();
    void Show();
};

class oCMapScreen {
public:

    virtual ~oCMapScreen();
    void Show(int map);
};

class oCStatusScreen {
public:

    virtual ~oCStatusScreen();
    void Show();
};

#endif
