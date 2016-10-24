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
#include <DxLib.h>
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

const double pi = 4.0 * std::atan(1.0);

// 万有引力定数
const double G = 1.0;

//-------- メイングラフィックハンドル
int main_graphic_handle;

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
        str[0] = '0' + (static_cast<int>(fps.real_sleep_time / 10) % 10);
        str[1] = '0' + (static_cast<int>(fps.real_sleep_time) % 10);
        str[2] = '.';
        str[3] = '0' + (static_cast<int>(fps.real_sleep_time * 10) % 10);
        str[4] = '0' + (static_cast<int>(fps.real_sleep_time * 100) % 10);
        str[5] = '0' + (static_cast<int>(fps.real_sleep_time * 1000) % 10);
        DrawString(0, 0, str, GetColor(0x00, 0x00, 0x00));
    }

    // 処理
    void operator ()(){
        ++frame_count;
        Sleep(static_cast<int>(real_sleep_time));
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

    // init DxLib
    if(
        SetOutApplicationLogValidFlag(FALSE) != 0 ||
        ChangeWindowMode(TRUE) != DX_CHANGESCREEN_OK ||
        SetGraphMode(window_width, window_height, 32) != DX_CHANGESCREEN_OK ||
        SetMainWindowText("gravity") != 0 ||
        DxLib_Init() != 0
    ){ return -1; }

    SetHookWinProc(window_proc);
    SetDrawScreen(DX_SCREEN_BACK);

    bool flag = true;

    while(true){
        if(ProcessMessage() == -1){
            break;
        }

        DrawBox(0, 0, window_width, window_height, GetColor(0xFF, 0xFF, 0xFF), TRUE);
        rikai::draw();
        ScreenFlip();

        if(flag){
            WaitKey();
            flag = false;
        }

        rikai::proc();
        fps();
    }

    DxLib_End();
    return 0;
}
