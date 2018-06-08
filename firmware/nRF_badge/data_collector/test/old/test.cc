#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <iostream>
#include <chrono>
#include <math.h>
#include <thread>
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <cstring>
#include <vector>


using namespace std;




template <class T>
class Dynarray
{
private:
	T *pa;
	int length;
	int nextIndex;
public:
	Dynarray();
	~Dynarray();
	T& operator[](int index);
	void add(int val);
	int size();
}; 

template <class T>
Dynarray<T>::Dynarray() {
	pa = new T[10];
	for (int i = 0; i < 10; i++) {
		Dynarray<int> d;
		pa[i] = d;
	}
	length = 10;
	nextIndex = 0;
}

template <class T>
Dynarray<T>::~Dynarray() {
	delete [] pa;
}


template <class T>
T& Dynarray<T>::operator[](int index) {
	T *pnewa;
	if (index >= length) {
		pnewa = new T[index + 10];
		for (int i = 0; i < nextIndex; i++) 
			pnewa[i] = pa[i];
		for (int j = nextIndex; j < index + 10; j++)
			pnewa[j] = 0;
		length = index + 10;
		delete [] pa;
		pa = pnewa;
	}
	
	// Added Michael
	//if (index > nextIndex || (index == 0 && nextIndex == 0))
	if (index > nextIndex || (index == nextIndex))
		nextIndex = index + 1;
	return *(pa + index);
}

template <class T>
void Dynarray<T>::add(int val) {
	T *pnewa;
	if (nextIndex == length) {
		length = length + 10;
		pnewa = new T[length];
		for (int i = 0; i < nextIndex; i++) {
			pnewa[i] = pa[i];
		}
		for (int j = nextIndex; j < length; j++) {
			Dynarray<int> d;
			pnewa[j] = d;
		}
		delete [] pa;
		pa = pnewa;
	}
	pa[nextIndex++] = val;
}
/*
template <class T>
int Dynarray<T>::size() {
	return length; 
}
*/

template <class T>
int Dynarray<T>::size() {
	return nextIndex; 
}


typedef struct test_struct {
	int a;	
	Dynarray<Dynarray<int>>* array;
} test_struct;



int main(void)
{

	std::vector<std::vector<int>> matrix;
	
	std::vector<int> row;
	matrix.push_back(row);
	
	matrix[0].push_back(2);
	
	
	
	std::cout << "Size: " << matrix.size() << "\n";
	std::cout << "Data: " << matrix[0][0] << "\n";
	
	
	
	
	/*
	Dynarray<int> da1;
	da1[0] = (2);
	//da1[10] = 5;
	
	for(int i = 0; i < da1.size(); i++) {
		std::cout << "Element [" << i << "]: " << da1[i] << "\n";	
	}
	
	std::cout << "Size: " << da1.size() << "\n";
	
	test_struct s;
	s.a = 10;
	s.array = &da1;
	
	
	
	std::cout << "Data: " << s.a << "\n";
	
	
	(*s.array)[20] = 5;
	std::cout << "Size: " << (*s.array).size() << "\n";
	
	*/
	
}









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

/*
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




//https://docs.oracle.com/cd/E19455-01/806-5257/attrib-16/index.html
//https://access.redhat.com/documentation/en-US/Red_Hat_Enterprise_MRG/2/html/Realtime_Reference_Guide/chap-Priorities_and_policies.html
//http://www.yonch.com/tech/82-linux-thread-priority
//https://www.tutorialspoint.com/cplusplus/cpp_multithreading.htm

pthread_attr_t tattr;
pthread_t tid, tid1;

int ret;
int newprio = 0;
sched_param param;

void* status;


Timer tmr;

void *threadFunction(void * p_context){
	
	//sleep(1);
	
	for(int j = 0; j < 10; j++) {
		for(volatile int i =0 ; i < 100000; i++) {
			
		}
		unsigned long long t = tmr.elapsed();
		std::cout << "Thread here! " << t << "\n" ;
	}
	
	
	
	
	
	
	
	pthread_exit(NULL);
}

void *threadFunction1(void * p_context){
	//	sleep(1);
	
	for(int j = 0; j < 10; j++) {
		for(volatile int i =0 ; i < 100000; i++) {
			
		}
		unsigned long long t = tmr.elapsed();
		std::cout << "Thread1 here! " << t << "\n" ;
	}
	
	pthread_exit(NULL);
}


int main(void)
{
	
	
	ret = pthread_attr_init(&tattr);
	
	ret = pthread_attr_getschedparam(&tattr, &param);
	
	
	param.sched_priority = newprio - 5;
	
	ret = pthread_attr_setschedparam(&tattr, &param);
	
	ret = pthread_create(&tid1, &tattr, threadFunction1, NULL);
	
	pthread_attr_destroy(&tattr);

	
	
	
	
	// Create thread with other priority:
	ret = pthread_attr_init(&tattr);
	
	ret = pthread_attr_getschedparam(&tattr, &param);
	
	
	param.sched_priority = newprio;
	
	ret = pthread_attr_setschedparam(&tattr, &param);
	
	ret = pthread_create(&tid, &tattr, threadFunction, NULL);
	
	pthread_attr_destroy(&tattr);
	
	
	
	
	
	
	
	
	std::cout << "Main running" << "\n";
	
	
	pthread_join(tid1, &status);
	pthread_join(tid, &status);
	

	
	
	
	//int pthread_setschedparam(pthread_t thread, int policy, const struct sched_param *param);
    //int pthread_getschedparam(pthread_t thread, int *policy, struct sched_param *param);


	
//	sched_get_priority_max(SCHED_FIFO);
	
	//std::cout << "Thread Priority: " <<  11 << "\n";
	//param.sched_priority
	
	//pthread_exit(NULL);
	return 0;
	
} 

*/
















// Compile with this: g++-4.6 -std=c++0x test.cc


// http://de.cppreference.com/w/cpp/chrono/duration

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

/*
void time_meas(void) {
	
	std::cout << "Thread running!\n";
	
	Timer tmr;// = Timer();
	
	unsigned long long t = tmr.elapsed();
	
	//tmr.reset();
    volatile uint32_t j = 0;
	double a = 0;
    for(j = 0; j < 1000000; j++) {
		//a = sin(a);
	}
    //std::cout << "afasdfasdfs" << std::endl;

    t = tmr.elapsed();
    std::cout << t << std::endl;
	
}


int main(void)
{
	
	
	std::thread first (time_meas);
	
	std::cout << "MAIN running!\n";
	
	
	first.join();
	
	
	return 0;
	
} // LCOV_EXCL_LINE
*/


/*
//http://www.gnu.org/software/pth/pth-manual.html#name
//http://www.linuxfromscratch.org/blfs/view/svn/general/pth.html

//#include <pth.h>
//#include <time.h>


 static void *ticker(void *_arg)
 {
     time_t now;
     char *ct;
     float load;
	int i = 0;
     for (i = 0; i < 2; i++) {
         //pth_sleep(5);
		 int volatile j = 0;
		 for(j = 0; j < 0xFFFFFFFF; j++);
         now = time(NULL);
         ct = ctime(&now);
         ct[strlen(ct)-1] = '\0';
         pth_ctrl(PTH_CTRL_GETAVLOAD, &load);
         std::cout << "ticker: time: " << ct << "average load:" << load << "\n";
     }
 }
 
 static void *ticker1(void *_arg)
 {
     time_t now;
     char *ct;
     float load;
	int i = 0;
     for (i = 0; i < 2; i++) {
         //pth_sleep(5);
		  int volatile j = 0;
		 for(j = 0; j < 0xFFFFFFFF; j++);
         now = time(NULL);
         ct = ctime(&now);
         ct[strlen(ct)-1] = '\0';
         pth_ctrl(PTH_CTRL_GETAVLOAD, &load);
         std::cout << "ticker1: time: " << ct << "average load:" << load << "\n";
     }
 }




int main(void) {
	
	std::cout << "Main running!\n";
	
	std::cout << "Min Priority: " << PTH_PRIO_MIN << " Max Priority: " << PTH_PRIO_MAX << "\n";
	
	pth_init();
	
	pth_attr_t attr;
	
	void* status;
	
	
	attr = pth_attr_new();
	
	pth_attr_set(attr, PTH_ATTR_NAME, "ticker");
	pth_attr_set(attr, PTH_ATTR_PRIO, -5);
    pth_attr_set(attr, PTH_ATTR_STACK_SIZE, 64*1024);
    pth_attr_set(attr, PTH_ATTR_JOINABLE, TRUE);
    pth_t tid = pth_spawn(attr, ticker, NULL);
	
	if(tid == NULL) {
		std::cout << "Error creating Thread\n";
	}
	
	
	
	attr = pth_attr_new();
	
	pth_attr_set(attr, PTH_ATTR_NAME, "ticker1");
	pth_attr_set(attr, PTH_ATTR_PRIO, 4);
    pth_attr_set(attr, PTH_ATTR_STACK_SIZE, 64*1024);
    pth_attr_set(attr, PTH_ATTR_JOINABLE, TRUE);
    pth_t tid1 = pth_spawn(attr, ticker1, NULL);
	
	
	
	
	
	
	
	
	pth_join(tid, &status);
	pth_join(tid1, &status);
	
	
}

*/