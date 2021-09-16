#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <atomic>
#include <condition_variable>
#include <future>

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

namespace demo_condition_variable{
	mutex mtx;
	condition_variable cond;
	atomic<bool> isEmpty(true);
	
	void producer(int n, queue<int> &q){
		for (int i = 0; i < n; i++){
			{
				lock_guard<mutex> lck(mtx);
				cerr << "producer: " << i << endl;
				q.push(i);
				isEmpty = false;
				cond.notify_one();
			}
			//this_thread::sleep_for(chrono::milliseconds(10));
		}
	}
	
	void consumer(int id, queue<int> &q){
		while (true){
			{
				unique_lock<mutex> lck(mtx);
				cond.wait(lck, [isEmpty]{return !isEmpty;});
				int i = q.front();
				q.pop();
				isEmpty = q.empty();
				cerr << "consumer" <<id << ": " << i << endl;
			}
			//this_thread::sleep_for(chrono::milliseconds(2));
		}
	}
	
	void demo(){
		queue<int> q;
		thread tp(producer, 100, ref(q));
		thread t1(consumer, 1, ref(q));
		thread t2(consumer, 2, ref(q));
		tp.join();
		t1.join();
		t2.join();
	}
}

namespace demo_future{
	void demo(){
		future<thread::id> result = async(launch::async, []{this_thread::sleep_for(chrono::milliseconds(2000)); return this_thread::get_id();});
		cout << "main thread id: " << this_thread::get_id() << endl;
		cout << result.get() << endl;
	}
	
	void demo2(){
		future<thread::id> result = async(launch::deferred, []{this_thread::sleep_for(chrono::milliseconds(2000)); return this_thread::get_id();});
		cout << "main thread id: " << this_thread::get_id() << endl;
		cout << result.get() << endl;
	}
	
	void demo3(){
		future<thread::id> result = async(launch::deferred, []{this_thread::sleep_for(chrono::milliseconds(2000)); return this_thread::get_id();});
		cout << "main thread id: " << this_thread::get_id() << endl;
		thread([](future<thread::id> x){cout << x.get() << endl;}, move(result)).join();
	}
	
	void demo4(){
		packaged_task<thread::id()> task([]{this_thread::sleep_for(chrono::milliseconds(2000)); return this_thread::get_id();});
		future<thread::id> result = task.get_future();
		cout << "main thread id: " << this_thread::get_id() << endl;
		thread(move(task)).detach();
		cout << "working thread id: " << result.get() << endl;
	}
	
	void demo5(){
		promise<thread::id> result;
		cout << "main thread id: " << this_thread::get_id() << endl;
		thread([](promise<thread::id> &t){this_thread::sleep_for(chrono::milliseconds(2000)); t.set_value(this_thread::get_id());}, ref(result)).detach();
		cout << result.get_future().get() << endl;
	}
}

#include<memory>
#include<functional>

typedef int score_t;

class ThreadPool{
	struct TaskBlock{
		TaskBlock(): isTermination(true){}
		TaskBlock(const shared_ptr<const vector<function<score_t(const int)> > > &pFuncs, promise<vector<score_t> > results, future<TaskBlock> next):
			isTermination(false), pFuncs(pFuncs), results(move(results)), next(move(next)){}
		
		bool isTermination;
		const shared_ptr<const vector<function<score_t(const int)> > > pFuncs;
		promise<vector<score_t> > results;
		future<TaskBlock> next;
	};
	
	static void worker(future<TaskBlock> next, const int id){
		while (true){
			TaskBlock task = next.get();
			if (task.isTermination) break;
			vector<score_t> results;
			for (const function<score_t(const int)> &f: *(task.pFuncs)){
				results.push_back(f(id));
			}
			task.results.set_value(move(results));
			next = move(task.next);
		}
	}
	
	void work(){
		const shared_ptr<const vector<function<score_t(const int)> > > pFuncs(new const vector<function<score_t(const int)> >(move(funcs)));
		funcs = vector<function<score_t(const int)> >();
		vector<future<vector<score_t> > > results;
		for (promise<TaskBlock> &task: tasks){
			promise<TaskBlock> curTask = move(task);
			task = promise<TaskBlock>();
			promise<vector<score_t> > res;
			results.push_back(move(res.get_future()));
			curTask.set_value(TaskBlock(pFuncs, move(res), task.get_future()));
		}
		vector<vector<score_t> > resultValues;
		resultValues.emplace_back();
		for (const function<score_t(const int)> &f: *(pFuncs)){
			resultValues[0].push_back(f(0));
		}
		for (future<vector<score_t> > &result: results){
			resultValues.push_back(result.get());
		}
		for (int i = 0; i < resultValues[0].size(); i++){
			score_t s = 0;
			for (vector<score_t> &rv: resultValues) s += rv[i];
			sums.push(s);
		}
		
	}
	
	int nThreads;
	vector<promise<TaskBlock> > tasks;
	vector<function<score_t(const int)> > funcs;
	queue<score_t> sums;
	
public:
	ThreadPool(int n): nThreads(n - 1), tasks(n - 1){
		for (int id = 1; id < n; id++){
			thread(worker, tasks[id - 1].get_future(), id).detach();
		}
	}
	
	~ThreadPool(){
		for (promise<TaskBlock> &task: tasks){
			task.set_value(TaskBlock());
		}
	}
	
	void push(function<score_t(const int)> f){
		funcs.push_back(f);
	}
	
	score_t pop(){
		if (sums.empty()) work();
		score_t v = sums.front();
		sums.pop();
		return v;
	}
};

void demoThreadPool(){
	ThreadPool TP(10);
	TP.push([](int id){this_thread::sleep_for(chrono::milliseconds(id * 5)); cerr << (to_string(id) + ": x = " + to_string(id) + "\n"); return id;});
	TP.push([](int id){cerr << (to_string(id) + ": x^2 = " + to_string(id * id) + "\n"); return id * id;});
	cerr << "sum x: " << TP.pop() << endl;
	cerr << "sum x^2: " << TP.pop() << endl;
	TP.push([](int id){cerr << (to_string(id) + ": x^3 = " + to_string(id * id * id) + "\n"); return id * id * id;});
	cerr << "sum x^3: " << TP.pop() << endl;
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
	//demo_mutex::bug();
	//demo_condition_variable::demo();
	//demo_future::demo();
	//demo_future::demo2();
	//demo_future::demo3();
	//demo_future::demo4();
	//demo_future::demo5();
	demoThreadPool();
	return 0;
}
