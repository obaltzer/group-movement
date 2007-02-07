#include "TimeTracer.h"

CTimeTracer::CTimeTracer()
{
	mdAFOPT_construct_time = 0;
	mdAFOPT_count_time = 0;
	mdAFOPT_pushright_time = 0;

	mdHStruct_construct_time = 0;
	mdHStruct_count_time = 0;
	mdHStruct_pushright_time = 0;

	mdBucket_count_time = 0;

	mdInitial_pushright_time = 0;

	mdtotal_running_time = 0;

}

void CTimeTracer::AddAFOPTContructTime(double run_time)
{
	mdAFOPT_construct_time += run_time;
}

void CTimeTracer::AddAFOPTCountTime(double run_time)
{
	mdAFOPT_count_time += run_time;

}

void CTimeTracer::AddAFOPTPushrightTime(double run_time)
{
	mdAFOPT_pushright_time += run_time;
}

void CTimeTracer::AddHStructConstructTime(double run_time)
{
	mdHStruct_construct_time += run_time;
}

void CTimeTracer::AddHStructCountTime(double run_time)
{
	mdHStruct_count_time += run_time;
}

void CTimeTracer::AddHStructPushrightTime(double run_time)
{
	mdHStruct_pushright_time += run_time;
}

void CTimeTracer::PrintStatistics(FILE* out_file)
{
	fprintf(out_file, "%.2f\t", mdtotal_running_time);
	fprintf(out_file, "%.2f %.2f %.2f\t", mdInitial_count_time, mdInitial_construct_time, mdInitial_pushright_time);
	fprintf(out_file, "%.2f %.2f %.2f\t", mdAFOPT_count_time, mdAFOPT_construct_time, mdAFOPT_pushright_time);
	fprintf(out_file, "%.2f %.2f %.2f\t", mdHStruct_count_time, mdHStruct_construct_time, mdHStruct_pushright_time);
	fprintf(out_file, "%.2f\t", mdBucket_count_time);

}
