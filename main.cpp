#include <chrono>
#include <vector>
#include <map>
#include <forward_list>
#include <random>
#include <memory>
#include <ctime>
#include <cmath>
#include <Eigen/Core>
#include <Eigen/Geometry>
#ifdef _WIN32
#include <windows.h>
#define usleep(x) Sleep(x * 1000)
#else
#include <unistd.h>
#endif
#include <SDL.h>
#include <SDL_ttf.h>
#include "bmp.hpp"

double rnd(){
    static std::mt19937 mt(0x23242526);
    return (mt() % (1 << 30)) / static_cast<double>(1 << 30);
}

using namespace Eigen;

using vector2d = Eigen::Matrix<double,2,1,Eigen::DontAlign>;

// ウィンドウサイズ
int window_width = 0;
int window_height = 0;

SDL_Surface* canvas;
SDL_Renderer* render;
TTF_Font* font;

const double pi = 4.0 * std::atan(1.0);

// 万有引力定数
const double G = 1.0;

//-------- メイングラフィックハンドル
int main_graphic_handle;

SDL_Color GetColor(uint8_t r, uint8_t g, uint8_t b) {
    return SDL_Color{r, g, b};
}

void DrawString(int x, int y, const char* str, SDL_Color&& color) {
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, str, color);
    assert(surface != nullptr);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(render, surface);
    assert(texture != nullptr);
    SDL_FreeSurface(surface);
    SDL_Rect rect = surface->clip_rect;
    rect.x = x; rect.y = y;
    SDL_RenderCopy(render, texture, &surface->clip_rect, &rect);
}

void DrawLine(int x1, int y1, int x2, int y2, SDL_Color&& color) {
    SDL_SetRenderDrawColor(render, color.r, color.g, color.g, 255);
    SDL_RenderDrawLine(render, x1, y1, x2, y2);
}

void DrawBox(int x, int y, int width, int height, SDL_Color&& color, BOOL fill) {
    SDL_Rect rect = {x, y, width, height};
    SDL_SetRenderDrawColor(render, color.r, color.g, color.g, 255);
    if (fill) SDL_RenderFillRect(render, &rect);
    else SDL_RenderDrawRect(render, &rect);
}

void DrawPixel(int x, int y, SDL_Color&& color) {
    SDL_SetRenderDrawColor(render, color.r, color.g, color.g, 255);
    SDL_RenderDrawPoint(render, x, y);
}

//-------- FPSマネージャー
class fps_manager{
public:
    // 目標FPS
    const int target_fps = 60;

    // 修正秒率
    const int fix_time_ratio = 10;

    // 修正フレーム単位
    const int fix_frames_num = target_fps / fix_time_ratio;

    // スリープタイム
    const float sleep_time = 1000.0f / target_fps;

    // 表示
    void draw_sleep_time() const{
        char str[7] = { 0 };
        str[0] = '0' + (static_cast<int>(real_sleep_time / 10) % 10);
        str[1] = '0' + (static_cast<int>(real_sleep_time) % 10);
        str[2] = '.';
        str[3] = '0' + (static_cast<int>(real_sleep_time * 10) % 10);
        str[4] = '0' + (static_cast<int>(real_sleep_time * 100) % 10);
        str[5] = '0' + (static_cast<int>(real_sleep_time * 1000) % 10);
        DrawString(0, 0, str, GetColor(0x00, 0x00, 0x00));
    }

    // 処理
    void operator ()(){
        ++frame_count;
        usleep(static_cast<int>(real_sleep_time) / 1000);
        if(frame_count >= fix_frames_num){
            std::chrono::milliseconds real = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);
            real_sleep_time = sleep_time * (sleep_time * fix_frames_num) / static_cast<float>(real.count());
            if(real_sleep_time >= sleep_time){
                real_sleep_time = sleep_time;
            }
            start = std::chrono::high_resolution_clock::now();
            frame_count = 0;
        }
    }

    float real_sleep_time = 16;

private:
    int frame_count = 0;
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
} fps;

namespace rikai{
    struct point{
        vector2d coord;
        const point *linked[2] = { nullptr };
    };

    std::forward_list<point> point_list;
    std::unique_ptr<tt_legacy::bmp> pbmp;

    void init(){
        std::map<std::pair<int, int>, point*> addr_map;
        pbmp.reset(new tt_legacy::bmp("rikai.bmp"));
        tt_legacy::bmp &bmp(*pbmp);
        window_width = bmp.width() + 200;
        window_height = bmp.height() + 200;
        for(int y = 0; y < bmp.height(); ++y){
            for(int x = 0; x < bmp.width(); ++x){
                if(bmp.clr(x, y) == tt_legacy::bmp::rgb(0x00, 0x00, 0x00)){
                    point p;
                    p.coord[0] = x;
                    p.coord[1] = y;
                    point_list.push_front(p);
                    addr_map.insert(std::make_pair(std::make_pair(x, y), &point_list.front()));
                }
            }
        }

        // ミストにしたい場合はここをコメントアウト
        for(int y = 0; y < bmp.height(); ++y){
            for(int x = 0; x < bmp.width(); ++x){
                if(bmp.clr(x, y) == tt_legacy::bmp::rgb(0x00, 0x00, 0x00)){
                    point &p(*addr_map[std::make_pair(x, y)]);
                    if(bmp.clr(x + 1, y) == tt_legacy::bmp::rgb(0x00, 0x00, 0x00)){
                        point &q(*addr_map[std::make_pair(x + 1, y)]);;
                        p.linked[0] = &q;
                    }

                    if(bmp.clr(x, y + 1) == tt_legacy::bmp::rgb(0x00, 0x00, 0x00)){
                        point &q(*addr_map[std::make_pair(x, y + 1)]);;
                        p.linked[1] = &q;
                    }
                }
            }
        }

        for(point &p : point_list){
            p.coord[0] += window_width / 2 - bmp.width() / 2;
            p.coord[1] += window_height / 2 - bmp.height() / 2;
        }
    }

    double proc_count = 0;

    void proc(){
        proc_count += 0.5;
        for(point &p : point_list){
            p.coord[0] += ((rnd() - 0.5) / 0.5) * proc_count * 0.2;
            p.coord[1] += ((rnd() - 0.5) / 0.5) * proc_count * 0.2;
        }
    }

    void draw(){
        for(point &p : point_list){
            DrawPixel(p.coord[0], p.coord[1], GetColor(0x00, 0x00, 0x00));
            for(int i = 0; i < 2; ++i){
                if(p.linked[i]){
                    const point &q(*p.linked[i]);
                    DrawLine(p.coord[0], p.coord[1], q.coord[0], q.coord[1], GetColor(0x00, 0x00, 0x00));
                    DrawPixel(q.coord[0], q.coord[1], GetColor(0x00, 0x00, 0x00));
                }
            }
        }
    }
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp){
    switch(msg){
    default:
        return DefWindowProc(hwnd, msg, wp, lp);
    }

    return 0;
}

int WINAPI WinMain(HINSTANCE handle, HINSTANCE prev_handle, LPSTR lp_cmd, int n_cmd_show){
    rikai::init();

    SDL_Window *window;
    SDL_Event event;

    SDL_Init(SDL_INIT_EVERYTHING);
    TTF_Init();

    window = SDL_CreateWindow(
        "gravity",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        window_width,
        window_height,
        SDL_WINDOW_SHOWN);
    assert(window != nullptr);
    canvas = SDL_GetWindowSurface(window);
    assert(canvas != nullptr);
    render = SDL_CreateRenderer(window, -1, 0);
    assert(render != nullptr);
    font = TTF_OpenFont("ipag-mona.ttf", 15);
    assert(font != nullptr);

    bool flag = true;

    while(flag){
        if(SDL_PollEvent(&event)){
            switch(event.type){
            case SDL_QUIT:
                flag = false;
                break;
            default:
                break;
            }
        }

        DrawBox(0, 0, window_width, window_height, GetColor(0xFF, 0xFF, 0xFF), TRUE);
        rikai::draw();
        fps.draw_sleep_time();
        SDL_RenderPresent(render);

        rikai::proc();
        fps();
    }

    TTF_CloseFont(font);
    TTF_Quit();
    return 0;
}
