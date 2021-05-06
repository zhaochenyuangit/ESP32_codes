#include <sys/time.h>
#include "time.h"

short dummy_event[] = {4672, 4544, 4672, 4544, 4672, 4864, 4864, 4992,
                       4608, 4608, 4672, 4672, 4736, 4800, 5056, 5056,
                       4672, 4544, 4608, 4864, 4864, 4928, 4928, 4928,
                       4864, 5440, 5504, 5568, 5632, 5504, 5056, 5248,
                       4864, 6080, 5888, 5952, 5824, 5568, 4800, 4864,
                       4672, 5184, 5632, 5952, 6080, 5248, 4992, 4864,
                       4672, 4544, 4864, 4992, 4928, 4864, 4992, 4800,
                       4736, 4800, 4800, 4800, 4736, 4864, 4736, 4992};

/*0 is start, 1 is end*/
int performance_evaluation(int start_or_end)
{
    static struct timeval tik, tok;
    if (start_or_end)
    {
        gettimeofday(&tok, NULL);
        int eclipsed_time_ms = (tok.tv_sec - tik.tv_sec) * 1000 + (tok.tv_usec - tik.tv_usec) / 1000;
        return eclipsed_time_ms;
    }
    else
    {
        gettimeofday(&tik, NULL);
        return -1;
    }
}
