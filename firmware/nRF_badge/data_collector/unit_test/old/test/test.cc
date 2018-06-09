
#include <stdint.h>
#include <stdbool.h>
#include <iostream>
#include <chrono>
#include <math.h>

using namespace std;


// Compile with this: g++-4.6 -std=c++0x test.cc


// http://de.cppreference.com/w/cpp/chrono/duration
class Timer
{
	
	
private:
    typedef std::chrono::high_resolution_clock clock_;
    typedef std::chrono::duration<unsigned long long, std::micro > microsecond_;
    std::chrono::time_point<clock_> beg_;
	
public:
    Timer() : beg_(clock_::now()) {}
    void reset() { beg_ = clock_::now(); }
	
    unsigned long long elapsed() const {
        return std::chrono::duration_cast<microsecond_>
            (clock_::now() - beg_).count(); }


};
/*
class Timer
{
	
	
private:
    typedef std::chrono::high_resolution_clock clock_;
    typedef std::chrono::duration<unsigned long long, std::nano > nanosecond_;
    std::chrono::time_point<clock_> beg_;
	
public:
    Timer() : beg_(clock_::now()) {}
    void reset() { beg_ = clock_::now(); }
	
    unsigned long long elapsed() const {
        return std::chrono::duration_cast<nanosecond_>
            (clock_::now() - beg_).count(); }


};
*/


int main(void)
{
	
	Timer tmr;// = Timer();
	
	unsigned long long t = tmr.elapsed();
	
	tmr.reset();
    volatile uint32_t j = 0;
	double a = 0;
    for(j = 0; j < 1000000; j++) {
		//a = sin(a);
	}
    //std::cout << "afasdfasdfs" << std::endl;

    t = tmr.elapsed();
    std::cout << t << std::endl;
	

	return 0;
}