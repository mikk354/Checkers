#pragma once

enum class Response
{
    OK,     // подтверждение хода
    BACK,   // возврат на 1 ход
    REPLAY, // перезапуск
    QUIT,   // выход
    CELL    // выбор клетки
}; 
