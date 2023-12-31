/*
 * Implementation of the word_count interface using Pintos lists and pthreads.
 *
 * You may modify this file, and are expected to modify it.
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

#ifndef PINTOS_LIST
#error "PINTOS_LIST must be #define'd when compiling word_count_lp.c"
#endif

#ifndef PTHREADS
#error "PTHREADS must be #define'd when compiling word_count_lp.c"
#endif

#include "word_count.h"

void init_words(word_count_list_t* wclist) {
    list_init(&wclist->lst);
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&wclist->lock,&attr);
}

size_t len_words(word_count_list_t* wclist) {
    pthread_mutex_lock(&wclist->lock);
    size_t count = 0;
    struct list_elem* p;
    for(p = list_begin(&wclist->lst);p != list_end(&wclist->lst);p = list_next(p))
        ++count;
    pthread_mutex_unlock(&wclist->lock);
    return count;
}

word_count_t* find_word(word_count_list_t* wclist, char* word) {
    pthread_mutex_lock(&wclist->lock);
    word_count_t* ret = NULL;
    struct list_elem* p;
    for(p = list_begin(&wclist->lst);p != list_end(&wclist->lst);p = list_next(p))
    {
        word_count_t* wc = list_entry(p,word_count_t,elem);
        if(strcmp(wc->word,word) == 0)
        {
            ret = wc;
            break;
        }
    }
    pthread_mutex_unlock(&wclist->lock);
    return ret;
}

word_count_t* add_word(word_count_list_t* wclist, char* word) {
    pthread_mutex_lock(&wclist->lock);
    word_count_t* wc = find_word(wclist,word);
    if(wc != NULL)
    {
        ++wc->count;
    }
    else
    {
        wc = (word_count_t*)malloc(sizeof(word_count_t));
        if(wc == NULL)
            return NULL;
        wc->word = word;
        wc->count = 1;
        wc->elem.prev = NULL;
        wc->elem.next = NULL;
        list_push_front(&wclist->lst,&wc->elem);
    }
    pthread_mutex_unlock(&wclist->lock);
    return wc;
}

void fprint_words(word_count_list_t* wclist, FILE* outfile) {
    if(wclist == NULL || outfile == NULL)
        return;
    pthread_mutex_lock(&wclist->lock);
    struct list_elem* p;
    for(p = list_begin(&wclist->lst);p != list_end(&wclist->lst);p = list_next(p))
    {
        word_count_t* wc = list_entry(p,word_count_t,elem);
        fprintf(outfile,"%8d\t%s\n",wc->count,wc->word);
    }
    pthread_mutex_unlock(&wclist->lock);
}

static bool less_list(const struct list_elem* ewc1, const struct list_elem* ewc2, void* aux) {
    bool (*less)(const word_count_t*,const word_count_t*) = aux;
    word_count_t* wc1 = list_entry(ewc1,word_count_t,elem);
    word_count_t* wc2 = list_entry(ewc2,word_count_t,elem);
    return less(wc1,wc2);
}

void wordcount_sort(word_count_list_t* wclist,
                    bool less(const word_count_t*, const word_count_t*)) {
    pthread_mutex_lock(&wclist->lock);
    list_sort(&wclist->lst, less_list, less);
    pthread_mutex_unlock(&wclist->lock);
}
