//Ilie Dumitru
#include<graphics.h>
#include<cmath>
#include<cstdio>
#include<ctime>
#include<vector>
#define GAMEMENU 0
#define MAINMENU 1
#define OPTIONSMENU 2
#define PAUSEMENU 3
#define ACHIEVEMENTSMENU 4
#define ENDLESSGAMEMODE 0
#define RAPIDFIREGAMEMODE 1
#define REVERSEGAMEMODE 2
#define INSIDEOUTGAMEMODE 3
#define CATCHGAMEMODE 4

//If the game is too fast or too slow, modify these values
const float rotationGradient=0.05, movementGradient=-0.005, wallVelocity=0.5f, playerRotationScale=0.1f, playerMovementScale=1;
const int spawnRate=300;

//If you want a bigger/smaller window to play the game modify these values(I do not ensure that the game will function the same way)
const int WIDTH=600, HEIGHT=600;

const float D2R=M_PI/180, R2D=1/D2R, TAU=M_PI*2;
float radius=5;

bool isPressed(int key) {return GetKeyState(key) & 0x80000;}
inline int min(int a, int b) {return a+(b-a)*(b<a);}
inline int max(int a, int b) {return a+(b-a)*(b>a);}
float fastInverseSqrt(float x)
{
    int i;
    float a, b;

    a=x*0.5f;
    b=x;
    i=*(int*)&b;
    i=0x5f3759df-(i>>1);
    b=*(float*)&i;
    b*=1.5f-a*b*b;
    b*=1.5f-a*b*b;
    return b;
}
float preSin(float x)
{
    float r=0, f=1, xx=1;
    int i;
    for(i=1;i<21;++i)
    {
        f*=i;
        xx*=x;
        if(i%4==1)
            r+=xx/f;
        if(i%4==3)
            r-=xx/f;
    }
    return r;
}
float sinTable[720];
inline float sin(float alpha) {return sinTable[(((int)(alpha*R2D))%720+720)%720];}
inline float cos(float alpha) {return sinTable[(((int)(alpha*R2D)+180)%720+720)%720];}

struct poz
{
    float x, y;

    poz(float _x=0, float _y=0) : x(_x), y(_y) {}
};

float spawnRadius=0.5f/fastInverseSqrt(WIDTH*WIDTH+HEIGHT*HEIGHT);
int startTime, secondPaused=0, timePlayed=0;

struct player
{
    float angle, length;
    poz p;
    bool dead;

    player() : angle(0), length(0), p(0, 0), dead(0) {}
    virtual~player() {}
    void rotate(float ammount) {this->angle+=ammount*D2R*spawnRadius/this->length*playerRotationScale; p.x=cos(this->angle)*this->length+(WIDTH>>1); p.y=sin(this->angle)*this->length+(HEIGHT>>1);}
    void move(float ammount) {this->length-=ammount*playerMovementScale; if(this->length>(WIDTH>>1)) this->length=WIDTH>>1; if(this->length>(HEIGHT>>1)) this->length=HEIGHT>>1; if(this->length<25)this->length=25; this->p.x=this->length*cos(this->angle)+(WIDTH>>1); this->p.y=this->length*sin(this->angle)+(HEIGHT>>1);}
    void draw() {setlinestyle(1, 0, 1); setcolor(COLOR(20, 200, 20)); setfillstyle(1, COLOR(20, 200, 20)); fillellipse(this->p.x, this->p.y, radius, radius);}
};

struct wall
{
    poz p;
    float length, angle, velocity;

    wall(float _angle) : length(spawnRadius), angle(_angle), velocity(0.5f+0.01*(time(0)-startTime)) {this->p.x=this->length*cos(this->angle); this->p.y=this->length*sin(this->angle);}
    virtual~wall() {}
    void move(float ammount=1) {this->length-=this->velocity*ammount; this->p.x=this->length*cos(this->angle); this->p.y=this->length*sin(this->angle);}
    void rotate(float ammount) {this->angle+=ammount*D2R; this->p.x=this->length*cos(this->angle); this->p.y=this->length*sin(this->angle);}
    void draw() {setlinestyle(1, 0, 5); setcolor(COLOR(200, 20, 20)); line((WIDTH>>1)+this->p.x+sin(angle)*length*0.7f, (HEIGHT>>1)+this->p.y-cos(angle)*length*0.7f, (WIDTH>>1)+this->p.x-sin(angle)*length*0.7f, (HEIGHT>>1)+this->p.y+cos(angle)*length*0.7f);}
    bool checkDeath(player& P)
    {
        poz dreapta(-cos(angle), -sin(angle)), playerPoz(P.p.x-this->p.x-(WIDTH>>1), P.p.y-this->p.y-(HEIGHT>>1));
        float dist=abs(dreapta.y*playerPoz.y+dreapta.x*playerPoz.x);
        if(dist<7.5f)
        {
            poz norm(dreapta.y, -dreapta.x);
            float num=dreapta.y*norm.x-dreapta.x*norm.y;
            num*=num;
            float normC=-(norm.x*playerPoz.x+norm.y*playerPoz.y);
            poz proj(-normC*dreapta.y, normC*dreapta.x);
            if(proj.x*proj.x+proj.y*proj.y<=(7.5f+this->length*0.7)*(7.5f+this->length*0.7)*num)
                return true;
        }
        return false;
    }
};

struct achievement
{
    bool completed, (*trigger)();
    char name[100];

    achievement(bool (*_trigger)()=0, const char* text=0) : completed(0) {this->trigger=_trigger; strcpy(this->name, text);}
    virtual~achievement() {this->trigger=0;}
    bool checkAcomplishment() {this->completed|=this->trigger(); return this->completed;}
};

struct button
{
    poz topleft, bottomright;
    void (*use)();
    char name[50];

    button(int left, int top, int right, int bottom, const char* _name, void (*_use)()=0) : topleft(left, top), bottomright(right, bottom) {this->use=_use; strcpy(this->name, _name);}
    button(poz tl, poz br, const char* _name, void (*_use)()=0) {this->bottomright=br; this->topleft=tl; this->use=_use; strcpy(this->name, _name);}
    virtual~button() {}
    bool check(int x, int y) {if(x>topleft.x && x<bottomright.x && y>topleft.y && y<bottomright.y) {this->use(); return true;} return false;}
    bool check(poz p) {if(p.x>topleft.x && p.x<bottomright.x && p.y>topleft.y && p.y<bottomright.y) {this->use(); return true;} return false;}
    void draw()
    {
        setfillstyle(1, COLOR(128, 128, 128));
        setcolor(WHITE);
        setlinestyle(0, 0, 1);
        bar(this->topleft.x, this->topleft.y, this->bottomright.x, this->bottomright.y);
        rectangle(this->topleft.x, this->topleft.y, this->bottomright.x, this->bottomright.y);
        setcolor(BLACK);
        setbkcolor(COLOR(128, 128, 128));
        outtextxy(((int)(this->topleft.x+this->bottomright.x-textwidth(this->name)))>>1, ((int)(this->topleft.y+this->bottomright.y-textheight(this->name)))>>1, this->name);
    }
};

struct menu
{
    std::vector<button> buttons;

    menu(int n, button* buttonsList)
    {
        int i=0;
        for(i=0;i<n;++i)
            this->buttons.push_back(buttonsList[i]);
    }
    virtual~menu() {this->buttons.clear();}
    void checkButtons() {int x=mousex(), y=mousey(); bool ok=true; std::vector<button>::iterator i0, i1; for(i0=this->buttons.begin(), i1=this->buttons.end();i0!=i1 && ok;++i0) ok=!i0->check(x, y);}
    void drawButtons() {std::vector<button>::iterator i0, i1; for(i0=this->buttons.begin(), i1=this->buttons.end();i0!=i1;++i0) i0->draw();}
};

std::vector<wall> v;
player P;

void normalUpdate()
{
    std::vector<wall>::iterator it0, it1;
    bool x;
    for(it0=v.begin(), it1=v.end();it0!=it1;it0+=!x)
    {
        it0->move();
        x=(it0->length<7.5);
        if(x)
        {
            it0=v.erase(it0);
            if(!v.size())
                it0=it1;
        }
    }
}

void insideOutUpdate()
{
    std::vector<wall>::iterator it0, it1;
    bool x;
    for(it0=v.begin(), it1=v.end();it0!=it1;it0+=!x)
    {
        it0->move(-1);
        x=(it0->length>=spawnRadius);
        if(x)
        {
            it0=v.erase(it0);
            if(!v.size())
                it0=it1;
        }
    }
}

void catchUpdate()
{
    std::vector<wall>::iterator it0, it1;
    bool x;
    for(it0=v.begin(), it1=v.end();it0!=it1;it0+=!x)
    {
        it0->move();
        x=it0->checkDeath(P);
        if(x)
        {
            it0=v.erase(it0);
            if(!v.size())
                it0=it1;
        }
    }
}

void normalDraw()
{
    std::vector<wall>::iterator it0, it1;
    P.draw();
    for(it0=v.begin(), it1=v.end();it0!=it1;++it0)
        it0->draw();
    setfillstyle(INTERLEAVE_FILL, COLOR(20, 20, 200));
    if(v.size())
    {
        setlinestyle(0, 0, 5);
        setcolor(COLOR(200, 20, 20));
    }
    else
    {
        setlinestyle(0, 0, 1);
        setcolor(COLOR(20, 20, 200));
    }
    fillellipse(WIDTH>>1, HEIGHT>>1, 15, 15);
}

bool checkDeath()
{
    std::vector<wall>::iterator it0, it1;
    bool x=0;
    P.draw();
    for(it0=v.begin(), it1=v.end();it0!=it1 && !x;++it0)
        x|=it0->checkDeath(P);
    return x;
}

void endScreen(int timeSurvived)
{
    char x[100];
    setbkcolor(BLACK);
    if(timeSurvived>=3600)
    {
        setcolor(COLOR(20, 200, 20));
        sprintf(x, "Congratulations! You survived for %d hour(s), %d minute(s) and %d second(s)!", timeSurvived/3600, timeSurvived/60%60, timeSurvived%60);
    }
    else if(timeSurvived>=60)
    {
        setcolor(YELLOW);
        sprintf(x, "Congratulations! You survived for %d minute(s) and %d second(s)!", timeSurvived/60, timeSurvived%60);
    }
    else
    {
        setcolor(WHITE);
        sprintf(x, "Congratulations! You survived for %d second(s)!", timeSurvived);
    }
    outtextxy((WIDTH-textwidth(x))>>1, (HEIGHT-textheight(x))>>1, x);
}

void loadData();
void saveData();
void checkAchievements();
void drawAchevementsCorner();
void drawAchevementsMenu();

void preload()
{
    int i;
    float f;
    for(i=f=0;i<720;++i, f+=D2R/2) sinTable[i]=preSin(f);
    loadData();
}

int achievementsMenuScrolled;

void resetStatistics();

const int menusCount=5;
int currentMenu=1, gamemode=ENDLESSGAMEMODE, lastSecond;
void playGameButtonUse()
{
    currentMenu=0;
    P.angle=0;
    P.length=0;
    P.dead=0;
    P.move(-1);
    v.clear();
    lastSecond=startTime=time(0);
}
void exitGameButtonUse() {currentMenu=-1;}
void optionsButtonUse() {currentMenu=OPTIONSMENU;}
void achievementsButtonUse() {currentMenu=ACHIEVEMENTSMENU; achievementsMenuScrolled=0;}
void endlessGameModeButtonUse() {gamemode=ENDLESSGAMEMODE;}
void rapidFireGameModeButtonUse() {gamemode=RAPIDFIREGAMEMODE;}
void reverseGameModeButtonUse() {gamemode=REVERSEGAMEMODE;}
void insideOutGameModeButtonUse() {gamemode=INSIDEOUTGAMEMODE;}
void catchGameModeButtonUse() {gamemode=CATCHGAMEMODE;}
void backToMainMenuButtonUse() {currentMenu=MAINMENU;}
void resetStatisticsButtonUse() {resetStatistics();}
void resumeGameButtonUse() {currentMenu=GAMEMENU; startTime+=time(0)-secondPaused;}
button mainMenuButtonList[]={button(((WIDTH-20)>>1)-35, HEIGHT/5-18, ((WIDTH-20)>>1)+35, HEIGHT/5+18, "Start", playGameButtonUse),
                             button(((WIDTH-20)>>1)-45, HEIGHT/5*2-18, ((WIDTH-20)>>1)+45, HEIGHT/5*2+18, "Options", optionsButtonUse),
                             button(((WIDTH-20)>>1)-66, HEIGHT/5*3-18, ((WIDTH-20)>>1)+65, HEIGHT/5*3+18, "Achievements", achievementsButtonUse),
                             button(((WIDTH-20)>>1)-35, HEIGHT/5*4-18, ((WIDTH-20)>>1)+35, HEIGHT/5*4+18, "Exit", exitGameButtonUse)},
       optionsMenuButtonList[]={button((WIDTH>>1)-37, HEIGHT/8-18, (WIDTH>>1)+36, HEIGHT/8+18, "Endless", endlessGameModeButtonUse),
                                button((WIDTH>>1)-44, HEIGHT/8*2-18, (WIDTH>>1)+43, HEIGHT/8*2+18, "Rapid Fire", rapidFireGameModeButtonUse),
                                button((WIDTH>>1)-38, HEIGHT/8*3-18, (WIDTH>>1)+37, HEIGHT/8*3+18, "Reverse", reverseGameModeButtonUse),
                                button((WIDTH>>1)-43, HEIGHT/8*4-18, (WIDTH>>1)+43, HEIGHT/8*4+18, "Inside Out", insideOutGameModeButtonUse),
                                button((WIDTH>>1)-28, HEIGHT/8*5-18, (WIDTH>>1)+28, HEIGHT/8*5+18, "Catch", catchGameModeButtonUse),
                                button((WIDTH>>1)-77, HEIGHT/8*6-18, (WIDTH>>1)+76, HEIGHT/8*6+18, "Reset achievements", resetStatisticsButtonUse),
                                button((WIDTH>>1)-26, HEIGHT/8*7-18, (WIDTH>>1)+26, HEIGHT/8*7+18, "Back", backToMainMenuButtonUse)},
       pauseMenuButtonList[]={button((WIDTH>>1)-36, 64, (WIDTH>>1)+36, 100, "Resume", resumeGameButtonUse),
                                button((WIDTH>>1)-25, HEIGHT-100, (WIDTH>>1)+25, HEIGHT-64, "Exit", backToMainMenuButtonUse)},
       achievementsMenuButtonList[]={button((WIDTH>>1)-25, HEIGHT-100, (WIDTH>>1)+25, HEIGHT-64, "Exit", backToMainMenuButtonUse)};
menu game(0, 0),
     mainMenu(4, mainMenuButtonList),
     optionsMenu(7, optionsMenuButtonList),
     pauseMenu(2, pauseMenuButtonList),
     achievementsMenu(1, achievementsMenuButtonList),
     menus[menusCount]={game, mainMenu, optionsMenu, pauseMenu, achievementsMenu};

const int achievementCount=15;

int main()
{
    preload();
    initwindow(WIDTH, HEIGHT, "Spiral", ((GetSystemMetrics(SM_CXSCREEN)-WIDTH)>>1)-3, ((GetSystemMetrics(SM_CYSCREEN)-HEIGHT)>>1)-26, true);
    wall w(0);
    srand(time(0));
    std::vector<wall>::iterator it0, it1;
    short ws, ad;
    bool lastPressedClick=false, currentPressedClick=false, lastPressedEscape=false, currentPressedEscape=false;
    int prevSecond=time(0), maxAchievementsMenuScroll=max(0, 60*(achievementCount-(HEIGHT-150)/60)-(HEIGHT-150)%60);
    while(currentMenu!=-1)
    {
        setbkcolor(BLACK);
        cleardevice();
        switch(currentMenu)
        {
        case GAMEMENU:
            {
                setbkcolor(BLACK);
                cleardevice();
                if(P.dead)
                {
                    normalDraw();
                    endScreen(lastSecond-startTime);
                    if(isPressed(VK_ESCAPE))
                        currentMenu=1;
                }
                else
                {
                    if(currentPressedEscape && !lastPressedEscape)
                    {
                        currentMenu=PAUSEMENU;
                        secondPaused=time(0);
                    }
                    else
                    {
                        switch(gamemode)
                        {
                        case ENDLESSGAMEMODE:
                            {
                                if(!(rand()%spawnRate))
                                {
                                    w.angle=rand()%720*D2R;
                                    w.length=spawnRadius;
                                    w.velocity=wallVelocity+0.002f*min(time(0)-startTime, 250);
                                    w.p.x=cos(w.angle)*w.length+(WIDTH>>1);
                                    w.p.y=sin(w.angle)*w.length+(HEIGHT>>1);
                                    v.push_back(w);
                                }
                                ws=isPressed('W')-isPressed('S');
                                P.move(ws+movementGradient);
                                ad=isPressed('A')-isPressed('D');
                                P.rotate(ad+rotationGradient);
                                for(it0=v.begin(), it1=v.end();it0!=it1;++it0)
                                    it0->rotate(rotationGradient);
                                normalUpdate();
                                normalDraw();
                            }
                            break;

                        case RAPIDFIREGAMEMODE:
                            {
                                if(++lastSecond>100)
                                {
                                    w.angle=rand()%720*D2R;
                                    w.length=spawnRadius;
                                    w.velocity=wallVelocity+0.002f*min(time(0)-startTime, 250);
                                    w.p.x=cos(w.angle)*w.length+(WIDTH>>1);
                                    w.p.y=sin(w.angle)*w.length+(HEIGHT>>1);
                                    v.push_back(w);
                                    lastSecond=0;
                                }
                                ws=isPressed('W')-isPressed('S');
                                P.move(ws+movementGradient);
                                ad=isPressed('A')-isPressed('D');
                                P.rotate(ad+rotationGradient);
                                for(it0=v.begin(), it1=v.end();it0!=it1;++it0)
                                    it0->rotate(rotationGradient);
                                normalUpdate();
                                normalDraw();
                            }
                            break;

                        case REVERSEGAMEMODE:
                            {
                                if(!(rand()%spawnRate))
                                {
                                    w.angle=rand()%720*D2R;
                                    w.length=spawnRadius;
                                    w.velocity=wallVelocity+0.002f*min(time(0)-startTime, 250);
                                    w.p.x=cos(w.angle)*w.length+(WIDTH>>1);
                                    w.p.y=sin(w.angle)*w.length+(HEIGHT>>1);
                                    v.push_back(w);
                                }
                                ws=isPressed('W')-isPressed('S');
                                P.move(-ws-movementGradient);
                                ad=isPressed('A')-isPressed('D');
                                P.rotate(-ad-rotationGradient);
                                for(it0=v.begin(), it1=v.end();it0!=it1;++it0)
                                    it0->rotate(rotationGradient);
                                normalUpdate();
                                normalDraw();
                            }
                            break;

                        case INSIDEOUTGAMEMODE:
                            {
                                if(!(rand()%spawnRate))
                                {
                                    w.angle=rand()%720*D2R;
                                    w.length=5;
                                    w.velocity=wallVelocity+0.002f*min(time(0)-startTime, 250);
                                    w.p.x=cos(w.angle)*w.length+(WIDTH>>1);
                                    w.p.y=sin(w.angle)*w.length+(HEIGHT>>1);
                                    v.push_back(w);
                                }
                                ws=isPressed('W')-isPressed('S');
                                P.move(ws+movementGradient);
                                ad=isPressed('A')-isPressed('D');
                                P.rotate(ad+rotationGradient);
                                for(it0=v.begin(), it1=v.end();it0!=it1;++it0)
                                    it0->rotate(rotationGradient);
                                insideOutUpdate();
                                normalDraw();
                            }
                            break;

                        case CATCHGAMEMODE:
                            {
                                if(!(rand()%spawnRate))
                                {
                                    w.angle=rand()%720*D2R;
                                    w.length=spawnRadius;
                                    w.velocity=wallVelocity+0.002f*min(time(0)-startTime, 250);
                                    w.p.x=cos(w.angle)*w.length+(WIDTH>>1);
                                    w.p.y=sin(w.angle)*w.length+(HEIGHT>>1);
                                    v.push_back(w);
                                }
                                ws=isPressed('W')-isPressed('S');
                                P.move(ws+movementGradient);
                                ad=isPressed('A')-isPressed('D');
                                P.rotate(ad+rotationGradient);
                                catchUpdate();
                                normalDraw();
                                for(it0=v.begin(), it1=v.end();it0!=it1 && !P.dead;it0+=!P.dead)
                                {
                                    it0->rotate(rotationGradient);
                                    if(it0->length<=7.5f)
                                    {
                                        P.dead=true;
                                        it0=v.erase(it0);
                                        if(!v.size())
                                            it0=it1;
                                    }
                                }
                            }
                            break;
                        }
                        P.dead|=checkDeath();
                        if(P.dead)
                            lastSecond=time(0);
                        checkAchievements();
                        drawAchevementsCorner();
                    }
                }
            }
            break;

        case MAINMENU:
            {

            }
            break;

        case OPTIONSMENU:
            {
                if(currentPressedEscape && !lastPressedEscape)
                    currentMenu=MAINMENU;
            }
            break;

        case PAUSEMENU:
            {
                if(currentPressedEscape && !lastPressedEscape)
                {
                    currentMenu=GAMEMENU;
                    startTime+=time(0)-secondPaused;
                }
            }
            break;

        case ACHIEVEMENTSMENU:
            {
                if(currentPressedEscape && !lastPressedEscape)
                    currentMenu=MAINMENU;
                if(isPressed(VK_DOWN))
                    achievementsMenuScrolled=min(achievementsMenuScrolled+1, maxAchievementsMenuScroll);
                if(isPressed(VK_UP))
                    achievementsMenuScrolled=max(0, achievementsMenuScrolled-1);
                drawAchevementsMenu();
            }
            break;
        }
        menus[currentMenu].drawButtons();
        swapbuffers();
        lastPressedClick=currentPressedClick;
        currentPressedClick=isPressed(VK_LBUTTON);
        lastPressedEscape=currentPressedEscape;
        currentPressedEscape=isPressed(VK_ESCAPE);
        if(currentPressedClick && !lastPressedClick)
            menus[currentMenu].checkButtons();
        timePlayed+=time(0)-prevSecond;
        prevSecond=time(0);
    }
    closegraph();
    saveData();
    return 0;
}

bool play1MinuteTrigger() {return gamemode==ENDLESSGAMEMODE && time(0)-startTime>59;}
bool play3MinuteTrigger() {return gamemode==ENDLESSGAMEMODE && time(0)-startTime>179;}
bool play5MinuteTrigger() {return gamemode==ENDLESSGAMEMODE && time(0)-startTime>299;}
bool play10MinuteTrigger() {return gamemode==ENDLESSGAMEMODE && time(0)-startTime>599;}
bool play1HourTrigger() {return time(0)-startTime+timePlayed>3599;}
bool play5HourTrigger() {return time(0)-startTime+timePlayed>17999;}
bool closeCallTrigger()
{
    std::vector<wall>::iterator i0, i1;
    for(i0=v.begin(), i1=v.end();i0!=i1;++i0)
    {
        poz dreapta(-cos(i0->angle), -sin(i0->angle)), playerPoz(P.p.x-i0->p.x-(WIDTH>>1), P.p.y-i0->p.y-(HEIGHT>>1));
        float dist=abs(dreapta.y*playerPoz.y+dreapta.x*playerPoz.x);
        if(dist<7.5f)
        {
            poz norm(dreapta.y, -dreapta.x);
            float num=dreapta.y*norm.x-dreapta.x*norm.y;
            num*=num;
            float normC=-(norm.x*playerPoz.x+norm.y*playerPoz.y);
            poz proj(-normC*dreapta.y, normC*dreapta.x);
            float dtc=proj.x*proj.x+proj.y*proj.y;
            if((dtc>(7.5f+i0->length*0.7)*(7.5f+i0->length*0.7)*num) && (dtc<(12.5f+i0->length*0.7)*(12.5f+i0->length*0.7)*num))
                return true;
        }
    }
    return false;
}
bool reverse15Trigger() {return gamemode==REVERSEGAMEMODE && time(0)-startTime>149;}
bool reverseMasterTrigger() {return gamemode==REVERSEGAMEMODE && time(0)-startTime>299;}
bool machineGunTrigger() {return gamemode==RAPIDFIREGAMEMODE && time(0)-startTime>149;}
bool ratatataTrigger() {return gamemode==RAPIDFIREGAMEMODE && time(0)-startTime>299;}
bool insideOutTrigger() {return gamemode==INSIDEOUTGAMEMODE && time(0)-startTime>39;}
bool catch1Trigger() {return gamemode==CATCHGAMEMODE && time(0)-startTime>149;}
bool catch2Trigger() {return gamemode==CATCHGAMEMODE && time(0)-startTime>299;}
bool catch3Trigger() {return gamemode==CATCHGAMEMODE && time(0)-startTime>599;}

achievement achievementList[achievementCount]={achievement(play1MinuteTrigger, "This is easy"),
                                               achievement(play3MinuteTrigger, "It's getting harder"),
                                               achievement(play5MinuteTrigger, "Master of the game"),
                                               achievement(play10MinuteTrigger, "God of the impossible"),
                                               achievement(play1HourTrigger, "Serious dedication"),
                                               achievement(play5HourTrigger, "No lifer"),
                                               achievement(closeCallTrigger, "That was close"),
                                               achievement(reverse15Trigger, "One step behind the other"),
                                               achievement(reverseMasterTrigger, "Palindrome"),
                                               achievement(machineGunTrigger, "Machine gun"),
                                               achievement(ratatataTrigger, "RATATATA..."),
                                               achievement(insideOutTrigger, "Inside out fan"),
                                               achievement(catch1Trigger, "Hey, son, catch!"),
                                               achievement(catch2Trigger, "Professional baseball mitt"),
                                               achievement(catch3Trigger, "Gotta catch 'em all")};
std::vector<achievement> achievementsDone;
std::vector<int> achievementsDoneTimeBegan;

void checkAchievements()
{
    int i;
    for(i=0;i<achievementCount;++i)
        if(!achievementList[i].completed)
            if(achievementList[i].checkAcomplishment())
                achievementsDone.push_back(achievementList[i]), achievementsDoneTimeBegan.push_back(time(0));
}

void drawAchevementsCorner()
{
    if(achievementsDone.size())
    {
        std::vector<achievement>::iterator ai0, ai1;
        std::vector<int>::iterator ti;
        bool x=false;
        int starty=0, stepy=50, endx;
        char v[150];
        for(ai0=achievementsDone.begin(), ai1=achievementsDone.end(), ti=achievementsDoneTimeBegan.begin();ai0!=ai1 && achievementsDone.size();ai0+=!x, ti+=!x, starty+=stepy)
        {
            setbkcolor(COLOR(60, 60, 60));
            setfillstyle(1, COLOR(60, 60, 60));
            setcolor(COLOR(200, 200, 200));
            setlinestyle(0, 0, 5);
            sprintf(v, "Well done! You achieved \"%s\"!", ai0->name);
            endx=textwidth(v)+8;
            bar(0, starty, endx, starty+stepy);
            rectangle(0, starty, endx, starty+stepy);
            setcolor(COLOR(20, 200, 20));
            outtextxy(4, starty+((stepy-textheight(v))>>1), v);
            x=time(0)-*ti>7;
            if(x)
            {
                ai0=achievementsDone.erase(ai0);
                ti=achievementsDoneTimeBegan.erase(ti);
            }
        }
    }
}

void loadData()
{
    FILE *f=fopen("game_data.dat", "r");
    int i;
    char x;
    fscanf(f, "%d", &timePlayed);
    for(i=0;i<achievementCount;++i)
        fscanf(f, "%c", &x), achievementList[i].completed=x-1;
    fclose(f);
}

void saveData()
{
    FILE *f=fopen("game_data.dat", "w");
    int i;
    fprintf(f, "%d", timePlayed);
    for(i=0;i<achievementCount;++i)
        fprintf(f, "%c", achievementList[i].completed+1);
    fclose(f);
}

void drawAchevementsMenu()
{
    int i, starty=50, stepy=40;
    setfillstyle(1, COLOR(127, 127, 200));
    setlinestyle(0, 0, 1);
    setbkcolor(COLOR(127, 127, 200));
    for(i=0;i<achievementCount && starty-achievementsMenuScrolled<HEIGHT;++i)
        if(achievementList[i].completed)
        {
            setcolor(COLOR(200, 200, 200));
            bar(50, starty-achievementsMenuScrolled, WIDTH-50, starty+stepy-achievementsMenuScrolled);
            rectangle(50, starty-achievementsMenuScrolled, WIDTH-50, starty+stepy-achievementsMenuScrolled);
            setbkcolor(COLOR(127, 127, 200));
            setcolor(COLOR(20, 200, 20));
            outtextxy(55, starty-achievementsMenuScrolled+((stepy-textheight(achievementList[i].name))>>1), achievementList[i].name);
            starty+=stepy+20;
        }
    setfillstyle(1, COLOR(63, 63, 63));
    setbkcolor(COLOR(63, 63, 63));
    for(i=0;i<achievementCount && starty-achievementsMenuScrolled<HEIGHT;++i)
        if(!achievementList[i].completed)
        {
            setcolor(COLOR(200, 200, 200));
            bar(50, starty-achievementsMenuScrolled, WIDTH-50, starty+stepy-achievementsMenuScrolled);
            rectangle(50, starty-achievementsMenuScrolled, WIDTH-50, starty+stepy-achievementsMenuScrolled);
            outtextxy(55, starty-achievementsMenuScrolled+((stepy-textheight(achievementList[i].name))>>1), achievementList[i].name);
            starty+=stepy+20;
        }
    setbkcolor(BLACK);
    setfillstyle(1, BLACK);
    bar(0, 0, WIDTH, 50);
    bar(0, HEIGHT-100, WIDTH, HEIGHT);
    setcolor(COLOR(20, 200, 20));
    outtextxy((WIDTH-textwidth((char*)"Achievements"))>>1, (50-textheight((char*)"Achievements"))>>1, (char*)"Achievements");
}

void resetStatistics()
{
    timePlayed=0;
    int i;
    for(i=0;i<achievementCount;++i)
        achievementList[i].completed=false;
}