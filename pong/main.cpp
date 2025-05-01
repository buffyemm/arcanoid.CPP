//linker::system::subsystem  - Windows(/ SUBSYSTEM:WINDOWS)
//configuration::advanced::character set - not set
//linker::input::additional dependensies Msimg32.lib; Winmm.lib

#include "windows.h" 
#include <algorithm> 
#include "math.h"

// секция данных игры  
typedef struct {
    float x, y, width, height, rad, dx, dy, speed;
    HBITMAP hBitmap;//хэндл к спрайту шарика 
    bool isActive;
} sprite;
const int line = 15, column = 7;
sprite racket;//ракетка игрока

sprite blocks[line][column];
sprite ball;//шарик

struct {
    int score, balls;//количество набранных очков и оставшихся "жизней"
    bool action = false;//состояние - ожидание (игрок должен нажать пробел) или игра
} game;

struct {
    HWND hWnd;//хэндл окна
    HDC device_context, context;// два контекста устройства (для буферизации)
    int width, height;//сюда сохраним размеры окна которое создаст программа
} window;

HBITMAP hBack;// хэндл для фонового изображения

//cекция кода

void InitGame()
{
    //в этой секции загружаем спрайты с помощью функций gdi
    //пути относительные - файлы должны лежать рядом с .exe 
    //результат работы LoadImageA сохраняет в хэндлах битмапов, рисование спрайтов будет произовдиться с помощью этих хэндлов
    ball.hBitmap = (HBITMAP)LoadImageA(NULL, "ball.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    racket.hBitmap = (HBITMAP)LoadImageA(NULL, "racket.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    auto blockBMP = (HBITMAP)LoadImageA(NULL, "bill.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    hBack = (HBITMAP)LoadImageA(NULL, "back.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    //------------------------------------------------------
    
    for (int i=0;i < line;i++) {
        for (int j = 0; j < column; j++) {
            blocks[i][j].width = window.width/line;
            blocks[i][j].height = window.height / 3 / column;
            blocks[i][j].x = blocks[i][j].width * i;
            blocks[i][j].y = blocks[i][j].height * j + window.height / 3;
            blocks[i][j].isActive = true;
            blocks[i][j].hBitmap = blockBMP;
        }
    }
    racket.width = 300;
    racket.height = 50;
    racket.speed = 30;//скорость перемещения ракетки
    racket.x = window.width / 2.;//ракетка посередине окна
    racket.y = window.height - racket.height;//чуть выше низа экрана - на высоту ракетки


    ball.dy = (rand() % 65 + 35) / 100.;//формируем вектор полета шарика
    ball.dx = -(1 - ball.dy);//формируем вектор полета шарика
    ball.speed = 300;
    ball.rad = 20;
    ball.x = racket.x;//x координата шарика - на середие ракетки
    ball.y = racket.y - ball.rad;//шарик лежит сверху ракетки

    game.score = 0;
    game.balls = 9;



}

void ProcessSound(const char* name)//проигрывание аудиофайла в формате .wav, файл должен лежать в той же папке где и программа
{
    PlaySound(TEXT(name), NULL, SND_FILENAME | SND_ASYNC);//переменная name содежрит имя файла. флаг ASYNC позволяет проигрывать звук паралельно с исполнением программы
}

void ShowScore()
{
    //поиграем шрифтами и цветами
    SetTextColor(window.context, RGB(160, 160, 160));
    SetBkColor(window.context, RGB(0, 0, 0));
    SetBkMode(window.context, TRANSPARENT);
    auto hFont = CreateFont(70, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, "CALIBRI");
    auto hTmp = (HFONT)SelectObject(window.context, hFont);

    char txt[32];//буфер для текста
    _itoa_s(game.score, txt, 10);//преобразование числовой переменной в текст. текст окажется в переменной txt
    TextOutA(window.context, 10, 10, "Score", 5);
    TextOutA(window.context, 200, 10, (LPCSTR)txt, strlen(txt));


    _itoa_s(game.balls, txt, 10);
    TextOutA(window.context, 10, 100, "Balls", 5);
    TextOutA(window.context, 200, 100, (LPCSTR)txt, strlen(txt));
}

void ProcessInput()
{
    if (GetAsyncKeyState(VK_LEFT)) racket.x -= racket.speed;
    if (GetAsyncKeyState(VK_RIGHT)) racket.x += racket.speed;

    if (!game.action && GetAsyncKeyState(VK_SPACE))
    {
        game.action = true;
        ProcessSound("bounce.wav");
    }
}

void ShowBitmap(HDC hDC, int x, int y, int x1, int y1, HBITMAP hBitmapBall, bool alpha = false)
{
                  
    HBITMAP hbm, hOldbm;
    HDC hMemDC;
    BITMAP bm;

    hMemDC = CreateCompatibleDC(hDC); // Создаем контекст памяти, совместимый с контекстом отображения
    hOldbm = (HBITMAP)SelectObject(hMemDC, hBitmapBall);// Выбираем изображение bitmap в контекст памяти

    if (hOldbm) // Если не было ошибок, продолжаем работу
    {
        GetObject(hBitmapBall, sizeof(BITMAP), (LPSTR)&bm); // Определяем размеры изображения

        if (alpha)
        {
            TransparentBlt(window.context, x, y, x1, y1, hMemDC, 0, 0, x1, y1, RGB(0, 0, 0));//все пиксели черного цвета будут интепретированы как прозрачные
        }
        else
        {
            StretchBlt(hDC, x, y, x1, y1, hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY); // Рисуем изображение bitmap
        }

        SelectObject(hMemDC, hOldbm);// Восстанавливаем контекст памяти
    }

    DeleteDC(hMemDC); // Удаляем контекст памяти
}


void block_collision(float start_x, float start_y, float dx, float dy, int max_bounces) {
    if (max_bounces <= 0) return;

    float length = sqrt((dx * ball.speed) * (dx * ball.speed) +
        (dy * ball.speed) * (dy * ball.speed));

          float s = k / (float)lenght;
        float fx = ddx * ball.speed;
        float fy = ddy * ball.speed;
        float new_x = bx + fx * s;
        float new_y = by + fy * s;
        float fake_x = new_x + bx   * s;
        float fake_y = new_y * s ;
        float reflectDX = ddx, reflectDY = ddy;
        float colX = 0, colY = 0;
                SetPixel(window.context, new_x, new_y, RGB(255, 20, 147));
                //SetPixel(window.context, fake_x, new_y, RGB(173, 255, 47));
        for (int i = 0; i < line; i++) {
            for (int j = 0; j < column; j++) {
                if (blocks[i][j].isActive && !collisionHandled) { // Проверяем только если столкновение ещё не обработано

    // Цвет: красный для первого вектора, зелёный для отражений
    COLORREF color = (max_bounces == 3) ? RGB(255, 20, 147) : RGB(173, 255, 47);

    for (int k = 0; k < length; k++) {
        float s = k / (float)length;
        float new_x = start_x + fx * s;
        float new_y = start_y + fy * s;

        // Проверяем столкновение с блоками
        for (int i = 0; i < line; i++) {
            for (int j = 0; j < column; j++) {
                if (blocks[i][j].isActive) {
                    // Проверяем пересечение окружности (мяча) с прямоугольником (блоком)
                    float closestX = max(blocks[i][j].x, min(new_x, blocks[i][j].x + blocks[i][j].width));
                    float closestY = max(blocks[i][j].y, min(new_y, blocks[i][j].y + blocks[i][j].height));

                    float distanceX = new_x - closestX;
                    float distanceY = new_y - closestY;
                    float distanceSquared = distanceX * distanceX + distanceY * distanceY;

                    if (distanceSquared < (ball.rad * ball.rad)) {
                        // Определяем сторону столкновения
                        float overlapLeft = (new_x + ball.rad) - blocks[i][j].x;
                        float overlapRight = (blocks[i][j].x + blocks[i][j].width) - (new_x - ball.rad);
                        float overlapTop = (new_y + ball.rad) - blocks[i][j].y;
                        float overlapBottom = (blocks[i][j].y + blocks[i][j].height) - (new_y - ball.rad);

                        float minOverlapX = min(overlapLeft, overlapRight);
                        float minOverlapY = min(overlapTop, overlapBottom);

                        reflectDX = dx;
                        reflectDY = dy;

                        if (minOverlapX < minOverlapY) {
                            reflectDX *= -1;
                        }
                        else {
                            ProcessSound("bounce.wav");
                            ddy *= -1; // Отскок по вертикали
                            reflectDY *= -1;
                        }

                        collision_k = k;
                        collision_x = new_x;
                        collision_y = new_y;
                        //blocks[i][j].isActive = false; // Деактивируем блок
                        goto collision_found; // Выходим из вложенных циклов
                    }
                }
            }
        }
                       if (collisionHandled) {
                       float ref_x = reflectDX * ball.speed;
                       float ref_y = reflectDY * ball.speed;
                       fake_x = colX + ref_x * s;
                       fake_y = colY + ref_y * s;
                       SetPixel(window.context, fake_x, new_y, RGB(173, 255, 47));
                       }
    }
}

void rev() {
    block_collision(ball.x, ball.y, ball.dx, ball.dy, 3);
}

void ShowRacketAndBall()
{
    ShowBitmap(window.context, 0, 0, window.width, window.height, hBack); // задний фон
    ShowBitmap(window.context, racket.x - racket.width / 2., racket.y, racket.width, racket.height, racket.hBitmap); // ракетка игрока
    ShowBitmap(window.context, ball.x - ball.rad, ball.y - ball.rad, 2 * ball.rad, 2 * ball.rad, ball.hBitmap, true); // шарик

    // Рисуем линию, представляющую движение мяча
                        SetPixel(window.context, 200, 1400, (255, 20, 147));
    //MoveToEx(window.context, ball.x, ball.y, NULL); // Начальная точка - текущее положение мяча
    //LineTo(window.context, ball.x + ball.dx * ball.speed, ball.y + ball.dy*ball.speed); // Конечная точка - положение мяча после движения

    // Отрисовка блоков
    for (int i = 0; i < line; i++) {
        for (int j = 0; j < column; j++) {
            if (blocks[i][j].isActive) {
                ShowBitmap(window.context, blocks[i][j].x, blocks[i][j].y, blocks[i][j].width, blocks[i][j].height, blocks[i][j].hBitmap);
            }
        }
    }
}





void LimitRacket()
{
    racket.x = max(racket.x, racket.width / 2.);//если коодината левого угла ракетки меньше нуля, присвоим ей ноль
    racket.x = min(racket.x, window.width - racket.width / 2.);//аналогично для правого угла
}

void CheckWalls()
{
    if (ball.x < ball.rad || ball.x > window.width - ball.rad)
    {
        ball.dx *= -1;
        ProcessSound("bounce.wav");
    }
}

void CheckRoof()
{

    if (ball.y < ball.rad)
    {
        ball.dy *= -1;
        ProcessSound("bounce.wav");
    }
}

bool tail = false;

void CheckFloor()
{
    if (ball.y > window.height - ball.rad - racket.height)//шарик пересек линию отскока - горизонталь ракетки
    {
        if (!tail && ball.x >= racket.x - racket.width / 2. - ball.rad && ball.x <= racket.x + racket.width / 2. + ball.rad)//шарик отбит, и мы не в режиме обработки хвоста
        {
            game.score++;//за каждое отбитие даем одно очко
           // ball.speed += 5. / game.score;//но увеличиваем сложность - прибавляем скорости шарику
            ball.dy *= -1;//отскок
            racket.width -= 10. / game.score;//дополнительно уменьшаем ширину ракетки - для сложности
            ProcessSound("bounce.wav");//играем звук отскока
        }
        else
        {//шарик не отбит

            tail = true;//дадим шарику упасть ниже ракетки

            if (ball.y - ball.rad > window.height)//если шарик ушел за пределы окна
            {
                game.balls--;//уменьшаем количество "жизней"

                ProcessSound("fail.wav");//играем звук

                if (game.balls < 0) { //проверка условия окончания "жизней"

                    MessageBoxA(window.hWnd, "game over", "", MB_OK);//выводим сообщение о проигрыше
                    InitGame();//переинициализируем игру
                }

                ball.dy = (rand() % 65 + 35) / 100.;//задаем новый случайный вектор для шарика
                ball.dx = -(1 - ball.dy);
                ball.x = racket.x;//инициализируем координаты шарика - ставим его на ракетку
                ball.y = racket.y - ball.rad;
                game.action = false;//приостанавливаем игру, пока игрок не нажмет пробел
                tail = false;
            }
        }
    }
}

void ProcessRoom()
{
    CheckWalls();
    CheckRoof();
    CheckFloor();
 //обрабатываем стены, потолок и пол. принцип - угол падения равен углу отражения, а значит, для отскока мы можем просто инвертировать часть вектора движения шарика
}

void ProcessBall()
{
    if (game.action)
    {
        //если игра в активном режиме - перемещаем шарик
        ball.x += ball.dx * ball.speed;
        ball.y += ball.dy * ball.speed;
    }
    else
    {
        //иначе - шарик "приклеен" к ракетке
        ball.x = racket.x;
    }
}

//void checkCollision() {
//    if(!tail && ball.x >= block - block.width / 2. - ball.rad && ball.x <= block.x + block.width / 2. + ball.rad)
//
//}

void InitWindow()
{
    SetProcessDPIAware();
    window.hWnd = CreateWindow("edit", 0, WS_POPUP | WS_VISIBLE | WS_MAXIMIZE, 0, 0, 0, 0, 0, 0, 0, 0);

    RECT r;
    GetClientRect(window.hWnd, &r);
    window.device_context = GetDC(window.hWnd);//из хэндла окна достаем хэндл контекста устройства для рисования
    window.width = r.right - r.left;//определяем размеры и сохраняем
    window.height = r.bottom - r.top;
    window.context = CreateCompatibleDC(window.device_context);//второй буфер
    SelectObject(window.context, CreateCompatibleBitmap(window.device_context, window.width, window.height));//привязываем окно к контексту
    GetClientRect(window.hWnd, &r);

}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{

    InitWindow();//здесь инициализируем все что нужно для рисования в окне
    InitGame();//здесь инициализируем переменные игры
   // mciSendString(TEXT("play ..\\Debug\\music.mp3 repeat"), NULL, 0, NULL);
    
    ShowCursor(NULL);

    while (!GetAsyncKeyState(VK_ESCAPE))
    {
        POINT p;
        GetCursorPos(&p);
        ScreenToClient(window.hWnd, &p);
        ball.x = p.x;
        ball.y = p.y;
        ShowRacketAndBall();
        ShowScore();//рисуем очик и жизни

        ProcessInput();//опрос клавиатуры
        LimitRacket();//проверяем, чтобы ракетка не убежала за экран
        //ProcessBall();//перемещаем шарик
        ProcessRoom();//обрабатываем отскоки от стен и каретки, попадание шарика в картетку
        rev();//рисуем фон, ракетку и шарик
        BitBlt(window.device_context, 0, 0, window.width, window.height, window.context, 0, 0, SRCCOPY);//копируем буфер в окно
        Sleep(60);//ждем 16 милисекунд (1/количество кадров в секунду)

    }

}
