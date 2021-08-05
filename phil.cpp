#include <iostream>
#include <Windows.h>
#include <vector>
#include <string>
#include <time.h>

const unsigned philAmount = 5;
HANDLE* Phils;
HANDLE supportThread = nullptr;

time_t startTime = 0;
time_t timeLimit = 0;
time_t philTime = 0;

HANDLE* semas;
HANDLE** philsSemas;
/* Потоки начинают есть парами. Возможны случаи:
 * 1 и 4,
 * 2 и 5,
 * 3 и 1,
 * 4 и 2,
 * 5 и 3.
 * Каждый философ может есть в паре с двумя другими философами:
 * Первый есть с четвертым, или третим,
 * Второй с четвертым или пятым,
 * Третий с первым или пятым,
 * Четвертый с первым или вторым,
 * Пятый с третим или вторым.
 *
 * Каждый философ ждет пока один из двух семафоров станет доступным для захвата. */

HANDLE* completeEvents;
HANDLE stdOutSema = nullptr;
HANDLE Start = nullptr;
unsigned stdoutCount = 0;

class threadInfo {
public:
    explicit threadInfo(const unsigned number) : threadNumber(number){}
    inline unsigned giveNumber () const { return threadNumber; }
private:
    threadInfo();
    unsigned threadNumber;
};
std::vector<threadInfo*> PhilInfos;

void message(const unsigned threadNumber, const bool eats){
    WaitForSingleObject(stdOutSema, INFINITE);

    if(eats)
//        std::cout << "Time: " << clock() << ", thread " << threadNumber + 1 << " eats" << std::endl;
        std::cout << clock() << ":" << threadNumber + 1 << ":T->E" << std::endl;
    else
//        std::cout << "Time: " << clock() << ", thread " << threadNumber + 1 << " thinks" << std::endl;
        std::cout << clock() << ":" << threadNumber + 1 << ":E->T" << std::endl;
    stdoutCount++;
    if(stdoutCount % 2 == 0)
        std::cout << std::endl;
    ReleaseSemaphore(stdOutSema, 1, nullptr);
}
int message(const std::string& message){
    WaitForSingleObject(stdOutSema, INFINITE);
    std::cout << message << std::endl;
    ReleaseSemaphore(stdOutSema, 1, nullptr);
    return 0;
}

DWORD WINAPI threadEntry(void* param) {
    const auto threadNumber = ((threadInfo*) param)->giveNumber();

    WaitForSingleObject(Start, INFINITE);

    while(true){
        if(clock() >= timeLimit + startTime)
            return 0;/*message("Thread " + std::to_string(threadNumber) + " ended up working");*/

        auto* tempSemas = philsSemas[threadNumber];
        const unsigned choise = WaitForMultipleObjects(2, tempSemas, false, INFINITE) - WAIT_OBJECT_0;

        message(threadNumber, true);

        Sleep(philTime);

        message(threadNumber, false);

        SetEvent(completeEvents[choise]);
    }
}

DWORD WINAPI supportEntry(void* param) {
    while(true){
        for(auto i = 0; i < philAmount; i++) {
            if(clock() >= timeLimit + startTime)
                return message("Time is over. Terminating...");

            ReleaseSemaphore(semas[i], 2, nullptr);
            WaitForMultipleObjects(2, completeEvents, true, INFINITE);
            ResetEvent(completeEvents[0]);
            ResetEvent(completeEvents[1]);
        }
    }
}

int createData(){
    semas = new HANDLE[philAmount];
    /* PhilAmount = 5:
     * 0 - one, forth
     * 1 - two, five
     * 2 - three, one
     * 3 - forth, two
     * 4 - five, three  */
    for(auto i = 0; i < philAmount; i++)
        semas[i] = CreateSemaphoreA(nullptr, 0, 2, nullptr);

    philsSemas = new HANDLE* [philAmount];
    for(auto i = 0; i < philAmount; i++) {
        philsSemas[i] = new HANDLE[2];
        switch(i){
            case 0:
                philsSemas[i][0] = semas[0];
                philsSemas[i][1] =          semas[2];
                break;
            case 1:
                philsSemas[i][0] = semas[1];
                philsSemas[i][1] =          semas[3];
                break;
            case 2:
                philsSemas[i][0] = semas[2];
                philsSemas[i][1] =          semas[4];
                break;
            case 3:
                philsSemas[i][0] = semas[3];
                philsSemas[i][1] =          semas[0];
                break;
            case 4:
                philsSemas[i][0] = semas[4];
                philsSemas[i][1] =          semas[1];
                break;
            default:
                return -1;
        }
    }

    completeEvents = new HANDLE[2];
    completeEvents[0] = CreateEventA(nullptr, true, false, nullptr);
    completeEvents[1] = CreateEventA(nullptr, true, false, nullptr);

    stdOutSema = CreateSemaphoreA(nullptr, 1, 1, nullptr);

    startTime = clock();

    Start = CreateEventA(nullptr, true, false, nullptr);

    /* ----------------------------------------------------- */
    supportThread = CreateThread(nullptr, 0, supportEntry, nullptr, 0, nullptr);

    Phils = new HANDLE[philAmount];
    PhilInfos.reserve(philAmount);
    for(auto i = 0; i < philAmount; i++){
        auto* temp = new threadInfo(i);
        PhilInfos.push_back(temp);
        Phils[i] = CreateThread(nullptr, 0, threadEntry, (void *) temp, 0, nullptr);
    }
    /* ----------------------------------------------------- */
    SetEvent(Start);
    return 0;
}
void deleteData(){
    WaitForSingleObject(supportThread, INFINITE);
    for(auto i = 0; i < philAmount; i++){
        CloseHandle(Phils[i]);
        delete PhilInfos[i];
    }
    PhilInfos.clear();
    PhilInfos.shrink_to_fit();
    delete [] Phils;

    CloseHandle(supportThread);
    CloseHandle(Start);
    CloseHandle(stdOutSema);

    CloseHandle(completeEvents[0]);
    CloseHandle(completeEvents[1]);
    delete [] completeEvents;

    for(auto i = 0; i < philAmount; i++) {
        philsSemas[i][0] = nullptr;
        philsSemas[i][1] = nullptr;
        delete [] philsSemas[i];
    }
    delete [] philsSemas;

    for(auto i = 0; i < philAmount; i++)
        CloseHandle(semas[i]);
    delete [] semas;
}

int main(const int argc, const char ** argv) {
    if(argc != 3) {
        std::cout << "Error with arguments" << std::endl;
        return -1;
    } else {
        timeLimit = std::stoi(argv[1]);
        philTime = std::stoi(argv[2]);
    }

    std::cout << "Time limit - " << timeLimit << std::endl;
    std::cout << "Time for one phil - " << philTime << std::endl << std::endl;
    createData();
    deleteData();
    return 0;
}