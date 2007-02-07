#ifndef _TIMETRACER_H
#define _TIMETRACER_H

#include <stdio.h>
#include <time.h>

class CTimeTracer
{
public:
	double mdAFOPT_construct_time;
	double mdAFOPT_count_time;
	double mdAFOPT_pushright_time;

	double mdHStruct_construct_time;
	double mdHStruct_count_time;
	double mdHStruct_pushright_time;

	double mdInitial_construct_time;
	double mdInitial_pushright_time;
	double mdInitial_count_time;
	double mdBucket_count_time;

	double mdpruning_time;

	double mdcfptree_build_time;
	double mdtotal_running_time;
	

	CTimeTracer();

	void AddAFOPTContructTime(double run_time);
	void AddAFOPTCountTime(double run_time);
	void AddAFOPTPushrightTime(double run_time);

	void AddHStructConstructTime(double run_time);
	void AddHStructCountTime(double run_time);
	void AddHStructPushrightTime(double run_time);
	
	void PrintStatistics(FILE* out_file);

};

extern CTimeTracer goTimeTracer;

#endif
