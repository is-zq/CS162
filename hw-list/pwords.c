/*
 * Word count application with one thread per input file.
 *
 * You may modify this file in any way you like, and are expected to modify it.
 * Your solution must read each input file from a separate thread. We encourage
 * you to make as few changes as necessary.
 */

/*
 * Copyright © 2021 University of California, Berkeley
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <pthread.h>

#include "word_count.h"
#include "word_helpers.h"

#define MAX_THREADS 10

struct ThreadArg
{
    word_count_list_t* word_counts;
    char file[128];
};

void* threadfun(void* arg)
{
    FILE* fp = fopen(((struct ThreadArg*)arg)->file,"r");
    count_words(((struct ThreadArg*)arg)->word_counts,fp);
    fclose(fp);

    pthread_exit(NULL);
}

/*
 * main - handle command line, spawning one thread per file.
 */
int main(int argc, char* argv[]) 
{
  /* Create the empty data structure. */
  word_count_list_t word_counts;
  init_words(&word_counts);

  pthread_t threads[MAX_THREADS+1];
  if (argc <= 1) 
  {
    /* Process stdin in a single thread. */
    count_words(&word_counts, stdin);
  }
  else 
  {
    for(int i=1;i<argc;i++)
    {
        struct ThreadArg arg;
        arg.word_counts = &word_counts;
        strcpy(arg.file,argv[i]);
        if(pthread_create(&threads[i],NULL,threadfun,(void*)&arg) != 0)
        {
            printf("pthread_create error\n");
            return -1;
        }
    }
  }

  for(int i=1;i<argc;i++)
      pthread_join(threads[i],NULL);

  /* Output final result of all threads' work. */
  wordcount_sort(&word_counts, less_count);
  fprint_words(&word_counts, stdout);
  return 0;
}
