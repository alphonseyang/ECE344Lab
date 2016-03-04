#ifndef _TEST_THREAD_H_
#define _TEST_THREAD_H_

void test_basic();
void test_preemptive();
void test_wakeup(int all);
void test_lock();
void test_cv_signal();
void test_cv_broadcast();

#endif /* _TEST_THREAD_H_ */
