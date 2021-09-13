#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <atomic>
using namespace std;

namespace demo_thread{
	void dummy(string s){
		cerr << (s + "\n"); 
		cerr << this_thread::get_id() << endl;
	}
	
	void demo(){
		cerr << this_thread::get_id() << endl;
		thread thrd(dummy, "Hello World!");
		thrd.join();
	}
	
	struct Dummy{
		int &i;
		Dummy(int &j): i(j){}
		operator()(){
				cerr << (string("Dummy " + to_string(i) + "\n"));
		}
	};
	
	thread create_thread(int i){
		int j = i;
		int *p = new int;
		*p = i;
		Dummy d(*p);
		thread thrd(d);
		delete p;
		return thrd;
	}
	
	void bug1(){
		vector<thread> thrds;
		for (int i = 0; i < 100; i++)
			thrds.push_back(create_thread(i));
		for (int i = 0; i < 100; i++)
			thrds[i].join();
	}
	
	void check(const int &i){
		cerr << &i << endl;
		for (int j = 0; j < 10; j++) cerr << (string("check " + to_string(i) + "\n"));
	}
	
	void bug2(){
		int i = 0;
		cerr << &i << endl;
		thread thrd(check, ref(i));
		cerr << (string("i becomes " + to_string(++i) + "\n"));
		thrd.join();
	}
	
	struct MyDouble{
		double v;
		MyDouble(const int &n){
			cerr << this_thread::get_id() << endl;
			v = n;
		}
	};
	
	void print_double(const MyDouble &d){
		cerr << (string("v = ") + to_string(d.v) + "\n");
	}
	
	thread cast_thread(int i){
		int j = i;
		thread thrd(print_double, MyDouble(j));	
		return thrd;
	}
	
	void bug3() {
		for (int i = 0; i < 10; i+=2){
			thread t1(cast_thread(i));
			thread t2(cast_thread(i+1));
			t1.join();
			t2.join();
		}
	}
	
	struct Counter{
		int cnt = 0, dummy;
		void count(){
			for (int i = 0; i < 1000; i++) {
				int temp = cnt;
				for (int j = 0; j < 10000; j++) temp+=j*j;
				for (int j = 0; j < 10000; j++) temp-=j*j;
				cnt = temp + 1;
				for (int j = 0; j < 10000; j++) temp+=j*j;
				for (int j = 0; j < 10000; j++) temp-=j*j;
				dummy = temp;
			}
		}
	};
	
	void bug4(){
		Counter c;
		thread t1(&Counter::count, &c);
		thread t2(&Counter::count, &c);
		t1.join();
		t2.join();
		cerr << c.cnt << endl;
	}
}

namespace demo_mutex{
	mutex mtx;
	
	struct Counter{
		int cnt = 0, dummy;
		void count(){
			for (int i = 0; i < 1000; i++) {
				mtx.lock();
				int temp = cnt;
				for (int j = 0; j < 7000; j++) temp+=j*j;
				for (int j = 0; j < 7000; j++) temp-=j*j;
				cnt = temp + 1;
				mtx.unlock();
				for (int j = 0; j < 7000; j++) temp+=j*j;
				for (int j = 0; j < 7000; j++) temp-=j*j;
				dummy = temp;
			}
		}
	};
	
	void demo(){
		Counter c;
		thread t1(&Counter::count, &c);
		thread t2(&Counter::count, &c);
		t1.join();
		t2.join();
		cerr << c.cnt << endl;
	}
	
	struct Counter2{
		int cnt = 0, dummy;
		void count(){
			for (int i = 0; i < 1000; i++) {
				int temp;
				{
					lock_guard<mutex> lg(mtx);
					temp = cnt;
					for (int j = 0; j < 7000; j++) temp+=j*j;
					for (int j = 0; j < 7000; j++) temp-=j*j;
					cnt = temp + 1;
				}
				for (int j = 0; j < 7000; j++) temp+=j*j;
				for (int j = 0; j < 7000; j++) temp-=j*j;
				dummy = temp;
			}
		}
	};
	
	void demo2(){
		Counter c;
		thread t1(&Counter::count, &c);
		thread t2(&Counter::count, &c);
		t1.join();
		t2.join();
		cerr << c.cnt << endl;
	}
	
	void producer(int n, queue<int> &q){
		for (int i = 0; i < n; i++){
			lock_guard<mutex> lg(mtx);
			cerr << "produce " << i << endl;
			q.push(i);
		}
	}
	
	void consumer(queue<int> &q){
		while (true){
			this_thread::sleep_for(chrono::milliseconds(5));
			{
				lock_guard<mutex> lg(mtx);
				if (!q.empty()){
					int i = q.front();
					q.pop();
					cerr << "consume " << i << endl;
				}
			}
		}
	}
	
	void demo3(){
		queue<int> q;
		thread tp(producer, 100, ref(q));
		thread t1(consumer, ref(q));
		thread t2(consumer, ref(q));
		tp.join();
		t1.join();
		t2.join();
	}
	
	atomic<bool> empty(true);
	
	void producer2(int n, queue<int> &q){
		for (int i = 0; i < n; i++){
			this_thread::sleep_for(chrono::milliseconds(50));
			{
				lock_guard<mutex> lg(mtx);
				cerr << "produce " << i << endl;
				q.push(i);
				empty = false;
			}
		}
	}
	
	void consumer2(queue<int> &q){
		while (true){
			if (!empty){
				cerr << q.empty() << endl;
				//this_thread::sleep_for(chrono::milliseconds(100));
				{
					lock_guard<mutex> lg(mtx);
					if (empty) continue;
					int i = q.front();
					q.pop();
					cerr << "consume " << i << endl;
					if (q.empty()) empty = true;
				}
			}
		}
	}
	
	void bug(){
		queue<int> q;
		thread tp(producer2, 100, ref(q));
		thread t1(consumer2, ref(q));
		thread t2(consumer2, ref(q));
		tp.join();
		t1.join();
		t2.join();
	}
}

int main(int argc, char** argv) {
	//demo_thread::demo();
	//demo_thread::bug1();
	//demo_thread::bug2();
	//demo_thread::bug3();
	//demo_thread::bug4();
	//demo_mutex::demo();
	//demo_mutex::demo2();
	//demo_mutex::demo3();
	demo_mutex::bug();
	return 0;
}
