/*
 *  Author : Yang Yuan  
 *           Department of Computer Science
 *           Peking University
 *
 *  This is a program for computing Treewidth. It runs on Windows. It uses the algorithm described in the paper 'A Fast Parallel Branch and Bound Algorithm for Treewidth', ICTAI 2011.
 *
 *  For any question about the algorithm or this program, feel free to contact the author: yyuanchn@gmail.com
 *
 *  08/22/2011
 */
#include <stdio.h>
#include <string>
#include <fstream>
#include <iostream>
#include <windows.h>
#include <tchar.h>
#define h 9999983   //hash table size
#define maxm (maxn*maxn)  
#define sum_limit 1000  //sum_limit is a heuristic used for allocate expanded nodes to free threads

//the parameters below should be set carefully according to the data
#define bits 32     //bits=32 or 64, machine-dependent
#define maxql 30000000   //maxql=maximum queue length, which is the ex array's length
#define maxn 150
#define max_bits 4  //max_bits*bits>=maxn 
#define Thread_s 4  //How many threads do you want to use? It depends on how many cores you have.
#define trick_mode false   //trick_mode means we only use simplicial vertex rule at the beginning of the search, 
                           //when this rule does not apply, we stop considering it, which saves the judge time. 
#define use_mode1 false    //use_mode1 means to use mmw() function to calculate lower bound every time, which is slow
#define use_mode2 false    //use_mode2 means add some addtitional ''safe'' edge to the graph during the search, 
                           //which may help reduce the search steps, but adding and removing these ''safe'' edges also require much time.
using namespace std;
//link saves search state
struct link
{
    int link_next;
    int state[max_bits];
    int lb;  //lowerbound
};


struct thread_node
{
    int thread_id;
    int next_thread;
};

bool forbid[maxn];
int re_order[maxn],order[maxn],cluster[maxn];
int pos_bit[maxn][2];
int n,m,ub;
int bitcode[bits];
bool entering;

int *table;
link *ex;  //the exist array, saves all the states we searched before

int free_p;
int free_thread;
int free_thread_left;
thread_node pool[Thread_s];
int update_thread_time;
long t1;

int g[Thread_s][maxn][maxn];
int next_same[Thread_s][maxn];
bool not_allow[Thread_s][maxn];
int state[Thread_s][max_bits];
int op[Thread_s][maxn];
int hash[Thread_s][maxn];
int map_hash[Thread_s][maxn][maxn];
int edge_add[Thread_s][maxm][2];
int edge_l[Thread_s];
int add_in[Thread_s][maxm][2];
int cur_add[Thread_s];
int lb[Thread_s];
int waiting_list[Thread_s][maxn];
int gran[Thread_s];
int link_[Thread_s][maxn];
int choice[Thread_s][maxn][maxn];
int eli[Thread_s][maxn];
char * filename;

int g_tmp[Thread_s][maxn][maxn];
int op_tmp[Thread_s][maxn];
int map_hash_tmp[Thread_s][maxn][maxn];
int remove_[Thread_s][maxn][maxn];
int restore[Thread_s][maxn][maxn];
int hash_num[max_bits];
int group[maxn];
int gp[maxn];

HANDLE end_signal;
//ub_mutex protects upperbound
//h_mutex protects ex array information
//thread_mutex protects threads information
CRITICAL_SECTION h_mutex,ub_mutex,thread_mutex;



int max (int a, int b)
{
    if (a>b)
        return a;
    else
        return b;
}
int min(int a, int b)
{
    if (a>b)
        return b;
    else 
        return a;
}

//check the clique condition
//if it is a clique, return 1
//if it is almost a clique, return 2
//else return 0
int clique(int thread, int cur)
{
    int i,j;
    int flag,ss;
    memset(link_[thread],0,sizeof(int)*(n+1));
    flag=0;
    ss=0;
    for (i=1;i<g[thread][cur][0];i++)
        for (j=i+1;j<=g[thread][cur][0];j++)
            if (map_hash[thread][g[thread][cur][i]][g[thread][cur][j]]==0)
            {
                flag++;
                link_[thread][g[thread][cur][i]]++;
                link_[thread][g[thread][cur][j]]++;
                if (link_[thread][g[thread][cur][i]]==2)
                    ss++;
                if (link_[thread][g[thread][cur][j]]==2)
                    ss++;
                if (ss>1)
                    return 0;
            }
    if (flag==0) return 1;
    return 2;
}

//this is a hash function
int make_pos(int thread)
{
    int i,s;
    s=0;
    for (i=0;i<max_bits;i++)
        s=(s+(state[thread][i]%h)*hash_num[i])%h;
    return (s+h)%h+1;
}

//check whether current state exists 
bool same(int thread,int where)
{
    int i;
    for (i=0;i<max_bits;i++)
        if (ex[where].state[i]!=state[thread][i])
            return false;
    return true;
}

//add a new state
//if added successfully, return true
//otherwise return false
bool add_state(int thread,int dep,int lb)
{
    state[thread][pos_bit[dep][0]]^=pos_bit[dep][1];
    int pos=make_pos(thread);
    int where=table[pos];
    
    //first, try to find current state in ex array
    while (where!=0)
    {
        if (same(thread,where))
        {
            if (ex[where].lb<=lb)
            {
                state[thread][pos_bit[dep][0]]^=pos_bit[dep][1];
                return false;
            }
            else
            {
                //if duplicated state exists, but needs to be updated, return true
                state[thread][pos_bit[dep][0]]^=pos_bit[dep][1];
                    ex[where].lb=lb;
                return true;
            }
        } else
            where=ex[where].link_next;
    }

    EnterCriticalSection(&h_mutex);
    //add a new state

        memcpy(ex[free_p].state,state[thread],sizeof(ex[free_p].state));
        ex[free_p].lb=lb;
        ex[free_p].link_next=table[pos];
        table[pos]=free_p;
        free_p++;

    LeaveCriticalSection(&h_mutex);

    //print message
    if (free_p>=maxql)
        printf("overflow!!\n");
    if (free_p%10000000==0)
    {
        printf("    free_p=%i",free_p);
        long t2=GetTickCount();
        printf("    uesd time=%lf\n",((double)t2-t1)/1000);
    }
        
    state[thread][pos_bit[dep][0]]^=pos_bit[dep][1];
    return true;
}

//check the simplicial condition
//if there is indeed a fixed vertex that should be eliminated next
//we check the future state, if it is duplicated, return -2
//otherwise return the vertex's  ID
//if the (almost) simplicial condition is not satisfied, return -1
int simplicial(int thread,int lowerb)
{
    int i,j;
    for (i=1;i<=op[thread][0];i++)
        if ((!forbid[op[thread][i]]) && (!not_allow[thread][op[thread][i]]))
        {
            j=clique(thread,op[thread][i]);
            if (j>0)
            {
                if (j==1)
                {
                    if (g[thread][op[thread][i]][0]<ub)
                    {
                        if (add_state(thread,op[thread][i],max(lowerb,g[thread][op[thread][i]][0])))
                            return op[thread][i];
                        else
                            return -2;
                    }
                    else 
                        return -2;
                }
                if ((j==2) && (g[thread][op[thread][i]][0]<=lowerb))
                {
                    if (add_state(thread,op[thread][i],lowerb))
                        return op[thread][i];
                    else 
                        return -2;
                }
            }
        }
    return -1;
}
//it is a heuristic to calculate lower bound, please check the paper
//I do not recommend to use it
int mmw(int thread)
{

    memcpy(op_tmp[thread],op[thread],sizeof(op[thread]));
    memcpy(g_tmp[thread],g[thread],sizeof(g[thread]));
    memcpy(map_hash_tmp[thread],map_hash[thread],sizeof(map_hash[thread]));

    /*
    int *op_tmp=(int*)malloc(sizeof(int)*(n+1));
    memcpy(op_tmp,op[thread],sizeof(int)*(n+1));
    int (*g_tmp)[maxn]=(int ((*)[maxn]))malloc(sizeof(int)*maxn*(n+1));
    memcpy(g_tmp,g[thread],sizeof(int)*maxn*(n+1));
    int (*map_hash_tmp)[maxn]=(int ((*)[maxn]))malloc(sizeof(int)*maxn*(n+1));
    memcpy(map_hash_tmp,map_hash[thread],sizeof(int)*maxn*(n+1));
    */

    int ans;
    int i,j,k;
    ans=0;
    for (i=op_tmp[thread][0];i>=2;i--)
    {
        int min=1;
        int cho;
        int min_common=n;
        for (j=2;j<=op_tmp[thread][0];j++)
            if (g_tmp[thread][op_tmp[thread][j]][0]<g_tmp[thread][op_tmp[thread][min]][0])
                min=j;
        cho=op_tmp[thread][min];
        op_tmp[thread][min]=op_tmp[thread][op_tmp[thread][0]];
        op_tmp[thread][0]--;
        min=cho;

        ans=max(ans,g_tmp[thread][min][0]);
        for (j=1;j<=g_tmp[thread][min][0];j++)
        {
            int common=0;
            for (k=1;k<=g_tmp[thread][min][0];k++)
                if ((map_hash_tmp[thread][min][g_tmp[thread][min][k]]>0) && 
                        (map_hash_tmp[thread][g_tmp[thread][min][j]][g_tmp[thread][min][k]]>0))
                        common++;
            if (common<min_common)
            {
                min_common=common;
                cho=g_tmp[thread][min][j];
            }
        }
        for (k=1;k<=g_tmp[thread][min][0];k++)
            if ((cho!=g_tmp[thread][min][k])&&(map_hash_tmp[thread][cho][g_tmp[thread][min][k]]==0))
            {
                g_tmp[thread][cho][0]++;
                g_tmp[thread][cho][g_tmp[thread][cho][0]]=g_tmp[thread][min][k];
                g_tmp[thread][g_tmp[thread][min][k]][0]++;
                g_tmp[thread][g_tmp[thread][min][k]][g_tmp[thread][g_tmp[thread][min][k]][0]]=cho;
                map_hash_tmp[thread][cho][g_tmp[thread][min][k]]=g_tmp[thread][cho][0];
                map_hash_tmp[thread][g_tmp[thread][min][k]][cho]=g_tmp[thread][g_tmp[thread][min][k]][0];
            }
        for (k=1;k<=g_tmp[thread][min][0];k++)
        {
            int tt=g_tmp[thread][min][k];
            map_hash_tmp[thread][tt][g_tmp[thread][tt][g_tmp[thread][tt][0]]]=map_hash_tmp[thread][tt][min];
            g_tmp[thread][tt][map_hash_tmp[thread][tt][min]]= g_tmp[thread][tt][g_tmp[thread][tt][0]];
            g_tmp[thread][tt][0]--;
        }
    }

    /*
    free(op_tmp);
    free(g_tmp);
    free(map_hash_tmp);
    */

    //return lower bound
    return ans;
}

//remove the edges that previously added
void removeedge(int thread)
{
    int i,a,b;
    for (i=0;i<edge_l[thread];i++)
    {
        a=edge_add[thread][i][0];
        b=edge_add[thread][i][1];
        g[thread][a][map_hash[thread][a][b]]=g[thread][a][g[thread][a][0]];
        g[thread][a][0]--;
        g[thread][b][map_hash[thread][b][a]]=g[thread][b][g[thread][b][0]];
        g[thread][b][0]--;
        map_hash[thread][a][b]=0;
        map_hash[thread][b][a]=0;
    }
}

//add same safe edges, it is a heuristic for reducing search steps, but is not quite efficient
void addedge(int thread)
{
    edge_l[thread]=0;
    int i,j,k;
    int list[maxn];
    list[0]=0;
    for (i=1;i<=op[thread][0];i++)
        if (g[thread][op[thread][i]][0]>=ub+1)
        {
            list[0]++;
            list[list[0]]=op[thread][i];
        }
    for (i=1;i<list[0];i++)
        for (j=i+1;j<=list[0];j++)
            if (map_hash[thread][list[i]][list[j]]==0)
            {
                int common=0;
                for (k=1;k<=g[thread][list[i]][0];k++)
                    if (map_hash[thread][list[j]][g[thread][list[i]][k]]>0)
                        common++;
                if (common>=ub+1)
                {
                    edge_add[thread][edge_l[thread]][0]=list[i];
                    edge_add[thread][edge_l[thread]][1]=list[j];
                    edge_l[thread]++;
                    g[thread][list[i]][0]++;
                    g[thread][list[i]][g[thread][list[i]][0]]=list[j];
                    g[thread][list[j]][0]++;
                    g[thread][list[j]][g[thread][list[j]][0]]=list[i];
                    map_hash[thread][list[i]][list[j]]=g[thread][list[i]][0];
                    map_hash[thread][list[j]][list[i]]=g[thread][list[j]][0];
                }
            }
}

//find better ub, update it, and write the file
void update_ub(int new_ub,int thread)
{
    EnterCriticalSection(&ub_mutex);
        if (ub>new_ub)
        {
            ub=new_ub;
            ofstream fout("rlt.txt");
            //printf("n=%i\n",n);
            fout<<filename<<endl;
            for (int i=n;i>op[thread][0];i--)
                fout<<order[eli[thread][i]]<<endl;
            fout<<-1<<endl;
            fout.close();
        }
    LeaveCriticalSection(&ub_mutex);
    printf("new ub=%i  ",ub);
    long t2=GetTickCount();
    printf("Time used: %.3lfs\n",((double)(t2-t1))/1000);
}

DWORD WINAPI begin_thread(LPVOID param);
//search the elimination order
//forced means the next vertex is chosen (=-1 means not fixed)
//sum is a heuristic used for allocating expanded nodes to free threads
//lowerb=lowerbound
void dfs(int thread,int forced,int sum,int lowerb) 
{
    int i;
    //if less than lowerb+1 vertices left, return
    if (op[thread][0]<=lowerb+1)
    {
        if (ub>lowerb)
            update_ub(lowerb,thread);
        return;
    }


    int label=op[thread][0];
    choice[thread][label][0]=0;
    if (forced==-1)
    {
        //update lowerbound, time consuming
        if (use_mode1)
            lowerb=max(lowerb,mmw(thread));
        //if lowerb==ub, stop search
        //otherwise, continue search
        if (lowerb<ub)
        {
            //add edge heuristic, time consuming
            if (use_mode2)
                addedge(thread);
            //entering is used for simplicial rule heuristic
            if (!entering)
            {
                int sim_rlt=simplicial(thread,lowerb);
                if (sim_rlt==-1)
                {
                    if (trick_mode)
                        entering=true;
                    //simplicial condition is not satisfied, try to find all potential vertices that could be eliminated
                    for (i=1;i<=op[thread][0];i++)
                        if ((!forbid[op[thread][i]]) && (g[thread][op[thread][i]][0]<ub)&& (!not_allow[thread][op[thread][i]]))
                            if (add_state(thread,op[thread][i],max(lowerb,g[thread][op[thread][i]][0])))
                            {
                                choice[thread][label][0]++;
                                choice[thread][label][choice[thread][label][0]]=op[thread][i];
                            }
                } else 
                    if (sim_rlt>-1)
                    {
                        //if the simplicial condition is satisfied, we choose that vertex
                        choice[thread][label][0]=1;
                        choice[thread][label][1]=sim_rlt;
                    }
            } else
            {
                // try to find all potential vertices that could be eliminated
                for (i=1;i<=op[thread][0];i++)
                    if ((!forbid[op[thread][i]]) && (g[thread][op[thread][i]][0]<ub)&& (!not_allow[thread][op[thread][i]]))
                        if (add_state(thread,op[thread][i],max(lowerb,g[thread][op[thread][i]][0])))
                        {
                            choice[thread][label][0]++;
                            choice[thread][label][choice[thread][label][0]]=op[thread][i];
                        }
            }
            if (use_mode2)
                removeedge(thread);
        }
    } else
    {
        //that means we do not need to choose the vertex to eliminate, because we have only one choice
        choice[thread][label][0]=1;
        choice[thread][label][1]=forced;
    }
    if (choice[thread][label][0]==0) return;
    if ((choice[thread][label][0]>1) && (sum<sum_limit) && (free_thread_left>0))
    {
        //if some condtions are meet, we allocate new expanded nodes to some free threads
        EnterCriticalSection(&thread_mutex);

            if (free_thread_left>0)
            {
                int use_thread=free_thread_left;
                if (use_thread>choice[thread][label][0]-1)
                    use_thread=choice[thread][label][0]-1;
                use_thread++;
                int choice_per_thread=choice[thread][label][0]/use_thread;
                int choice_residual=choice[thread][label][0]%use_thread;
                int cur_pos=1,cur_len=choice_per_thread+1;
                if (choice_residual==0)
                    cur_len--;
                for (int i=1;i<use_thread;i++)
                {
                    cur_pos+=cur_len;
                    if (choice_residual==i)
                        cur_len--;

                    int new_thread=pool[free_thread].thread_id;
                    update_thread_time++;
                    free_thread=pool[free_thread].next_thread;
                    memcpy(g[new_thread],g[thread],sizeof(g[thread]));
                    memcpy(next_same[new_thread],next_same[thread],sizeof(next_same[thread]));
                    memcpy(not_allow[new_thread],not_allow[thread],sizeof(not_allow[thread]));
                    memcpy(state[new_thread],state[thread],sizeof(state[thread]));
                    memcpy(op[new_thread],op[thread],sizeof(op[thread]));
                    memcpy(hash[new_thread],hash[thread],sizeof(hash[thread]));
                    memcpy(map_hash[new_thread],map_hash[thread],sizeof(map_hash[thread]));
                    memcpy(eli[new_thread],eli[thread],sizeof(eli[thread]));
                    cur_add[new_thread]=0;
                    lb[new_thread]=lowerb;
                    gran[new_thread]=sum*choice[thread][label][0];
                    waiting_list[new_thread][0]=cur_len;
                    memcpy(&waiting_list[new_thread][1],&choice[thread][label][cur_pos],sizeof(int)*cur_len);

                    CloseHandle(CreateThread(NULL,0,begin_thread,&pool[new_thread].thread_id,0,NULL));
                }
                free_thread_left-=use_thread-1;
                choice[thread][label][0]=choice_per_thread+1;
                if (choice_residual==0)
                    choice[thread][label][0]--;
            }
        LeaveCriticalSection(&thread_mutex);
    }
    if (sum<sum_limit) sum*=choice[thread][label][0];

    int j,op_last,interv[2],dep,cur_choice;
    // for each choice, we start a new DFS search
    for (cur_choice=1;cur_choice<=choice[thread][label][0];cur_choice++)
    {
        dep=choice[thread][label][cur_choice];
        eli[thread][label]=dep;

        //below we try to elminate current vertex, and change the graph
        //for how to eliminate a vertex, please see the paper
        //we also add some tricks for do the operation quickly
        op[thread][hash[thread][dep]]=op[thread][op[thread][0]];
        hash[thread][op[thread][op[thread][0]]]=hash[thread][dep];
        op_last=hash[thread][dep];
        hash[thread][dep]=0;
        op[thread][0]--;
        remove_[thread][label][0]=0;
        for (i=1;i<=g[thread][dep][0];i++)
        {
            remove_[thread][label][0]++;
            remove_[thread][label][remove_[thread][label][0]]=g[thread][dep][i];
        }
        interv[0]=cur_add[thread];
        for (i=1;i<remove_[thread][label][0];i++)
            for (j=i+1;j<=remove_[thread][label][0];j++)
                if (map_hash[thread][remove_[thread][label][i]][remove_[thread][label][j]]==0)
                {
                    add_in[thread][cur_add[thread]][0]=remove_[thread][label][i];
                    add_in[thread][cur_add[thread]][1]=remove_[thread][label][j];
                    g[thread][remove_[thread][label][i]][0]++;
                    g[thread][remove_[thread][label][i]][g[thread][remove_[thread][label][i]][0]]=remove_[thread][label][j];
                    map_hash[thread][remove_[thread][label][i]][remove_[thread][label][j]]=g[thread][remove_[thread][label][i]][0];
                    g[thread][remove_[thread][label][j]][0]++;
                    g[thread][remove_[thread][label][j]][g[thread][remove_[thread][label][j]][0]]=remove_[thread][label][i];
                    map_hash[thread][remove_[thread][label][j]][remove_[thread][label][i]]=g[thread][remove_[thread][label][j]][0];
                    cur_add[thread]++;
                }
        interv[1]=cur_add[thread];
        for (i=1;i<=remove_[thread][label][0];i++)
        {
            g[thread][remove_[thread][label][i]][map_hash[thread][remove_[thread][label][i]][dep]]=g[thread][remove_[thread][label][i]][g[thread][remove_[thread][label][i]][0]];
            g[thread][remove_[thread][label][i]][0]--;
            map_hash[thread][remove_[thread][label][i]][g[thread][remove_[thread][label][i]][map_hash[thread][remove_[thread][label][i]][dep]]]=map_hash[thread][remove_[thread][label][i]][dep];
            restore[thread][label][i]=map_hash[thread][remove_[thread][label][i]][dep];
            map_hash[thread][remove_[thread][label][i]][dep]=0;
        }
        state[thread][pos_bit[dep][0]]=state[thread][pos_bit[dep][0]]^pos_bit[dep][1];
        if (next_same[thread][dep]>0) not_allow[thread][next_same[thread][dep]]=false;

        //after eliminating the vertex, we enter the next layer
        dfs(thread,-1,sum,max(lowerb,g[thread][dep][0]));

        //below, as most DFS programs do, we change current graphs back, and add the vertex we eliminated previously into the graph, and check the next vertex
        if (next_same[thread][dep]>0) not_allow[thread][next_same[thread][dep]]=true;

        state[thread][pos_bit[dep][0]]=state[thread][pos_bit[dep][0]]^pos_bit[dep][1];

        hash[thread][dep]=op_last;
        op[thread][0]++;
        op[thread][op[thread][0]]=op[thread][op_last];
        op[thread][op_last]=dep;
        hash[thread][op[thread][op[thread][0]]]=op[thread][0];

        for (i=1;i<=remove_[thread][label][0];i++)
        {
            g[thread][remove_[thread][label][i]][0]++;
            g[thread][remove_[thread][label][i]][g[thread][remove_[thread][label][i]][0]]=g[thread][remove_[thread][label][i]][restore[thread][label][i]];
            g[thread][remove_[thread][label][i]][restore[thread][label][i]]=dep;
            map_hash[thread][remove_[thread][label][i]][dep]=restore[thread][label][i];
            map_hash[thread][remove_[thread][label][i]][g[thread][remove_[thread][label][i]][g[thread][remove_[thread][label][i]][0]]]=g[thread][remove_[thread][label][i]][0];
        }
        for (i=interv[0];i<interv[1];i++)
        {
            map_hash[thread][add_in[thread][i][0]][add_in[thread][i][1]]=0;
            map_hash[thread][add_in[thread][i][1]][add_in[thread][i][0]]=0;
            g[thread][add_in[thread][i][0]][0]--;
            g[thread][add_in[thread][i][1]][0]--;
        }
        cur_add[thread]=interv[0];
    }
}

//begin a new thread
DWORD WINAPI begin_thread(LPVOID param)
{
    int name=*(int*)param;
    if (waiting_list[name][0]>0)
    {
        for (int i=1;i<=waiting_list[name][0];i++)
            dfs(name,waiting_list[name][i],gran[name],lb[name]);
    } else
        dfs(name,-1,1,lb[name]);
    EnterCriticalSection(&thread_mutex);
    free_thread_left++;
    pool[name].next_thread=free_thread;
    free_thread=name;
    if (free_thread_left==Thread_s)
        ReleaseSemaphore(end_signal,1,NULL);
    LeaveCriticalSection(&thread_mutex);
    return 0;
}


//this program needs some other EXEs to do the pre-processing
int main(int argc, char* argv[])
{
    filename=new char[100];
    sprintf(filename,"instances\\%s.col",argv[1]);  //get the filename

    char temp[100];
    HANDLE process;
    LPTSTR szCmdline;

    //statements below call clique.exe to find the maximum clique (using simple DFS, which is sufficient in Treewidth problem) in the graph 
    sprintf(temp,"clique.exe %s",filename);
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    szCmdline=_tcsdup(TEXT(temp));
    ZeroMemory(&si,sizeof(si));
    si.cb=sizeof(si);
    ZeroMemory(&pi,sizeof(pi));
    CreateProcess(NULL,szCmdline,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi);
    process=pi.hProcess;
    WaitForSingleObject(process,INFINITE);
    TerminateProcess(process,0);
    CloseHandle(process);

    //statements below call rotate.exe to find similar groups
    sprintf(temp,"rotate.exe %s",filename);
    szCmdline=_tcsdup(TEXT(temp));
    ZeroMemory(&si,sizeof(si));
    si.cb=sizeof(si);
    ZeroMemory(&pi,sizeof(pi));
    CreateProcess(NULL,szCmdline,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi);
    process=pi.hProcess;
    WaitForSingleObject(process,INFINITE);
    TerminateProcess(process,0);
    CloseHandle(process);


    //statements below call dfs_ub.exe to calculate an upperbound of the treewidth
    //dfs_ub implements MFPBB to calculate upperbound quickly 
    //NOTE: an upperbound is not a must; but without a non-trivial upperbound, this program might be slower
    sprintf(temp,"dfs_ub.exe %s -1",argv[1]);
    szCmdline=_tcsdup(TEXT(temp));
    ZeroMemory(&si,sizeof(si));
    si.cb=sizeof(si);
    ZeroMemory(&pi,sizeof(pi));
    CreateProcess(NULL,szCmdline,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi);
    process=pi.hProcess;
    WaitForSingleObject(process,INFINITE);
    TerminateProcess(process,0);
    CloseHandle(process);

    //get the upperbound from the dfs_ub_output.exe
    string s;
    ifstream fin("dfs_ub_output.txt");
    getline(fin,s);
    sscanf(s.c_str(),"%i",&ub);
    fin.close();

    //read the clique information
    freopen("clique.txt","r",stdin);
    int clique_time;
    scanf("%i",&clique_time);
    int clique_num[maxn];
    scanf("%i",&clique_num[0]);
    int i,j,k;
    for (i=1;i<=clique_num[0];i++)
        scanf("%i",&clique_num[i]);

    t1=GetTickCount();
    fin.open(filename);
    //calculate 2-power numbers
    bitcode[0]=1;
    for (i=1;i<bits;i++)
        bitcode[i]=bitcode[i-1]*2;
    int m=0;
    //below we read the graph
    while (getline(fin,s))
    {
        if (s[0]=='p')
            sscanf(s.c_str(),"%*[^0-9]%i%i",&n,&i);
        if (s[0]=='e')
        {
            sscanf(s.c_str(),"%*[^0-9]%i%i",&i,&j);
            if (i>n)
                n=i;
            if (j>n)
                n=j;
            i--;
            j--;
            if (map_hash[0][i][j]==0)
            {
                m++;
                g[0][i][0]++;
                g[0][i][g[0][i][0]]=j;
                g[0][j][0]++;
                g[0][j][g[0][j][0]]=i;
                map_hash[0][i][j]=g[0][i][0];
                map_hash[0][j][i]=g[0][j][0];
            }
        }
    }
    fin.close();
    j=0;
    k=0;
    //map 0..n-1 into bitcode
    //bitcode helps us store graphs
    for (i=0;i<n;i++)
    {
        pos_bit[i][0]=j;
        pos_bit[i][1]=bitcode[k];
        k++;
        if (k>=bits)
        {
            k-=bits;
            j++;
        }
    }
    //order is an array which helps us to change the ordering of the nodes
    //a better ordering may accelerate the search
    for (i=0;i<n;i++)
        order[i]=i;
    for (i=0;i<n-1;i++)
        for (j=i+1;j<n;j++)
            if (g[0][order[i]][0]<g[0][order[j]][0])
            {
                k=order[i];
                order[i]=order[j];
                order[j]=k;
            }
    for (i=0;i<n;i++)
        re_order[order[i]]=i;

    memset(group,0,sizeof(group));
    memset(gp,0,sizeof(gp));
    int g_count=0;
    //read the similar groups, and initialize the forbid array, which fixes an order during the search in each similar group
    freopen("rotate.txt","r",stdin);
    while (true)
    {
        scanf("%i",&cluster[0]);
        if (cluster[0]==-1)
            break;
        g_count++;
        for (i=1;i<=cluster[0];i++)
        {
            scanf("%i",&cluster[i]);
            cluster[i]=re_order[cluster[i]];
            group[cluster[i]]=g_count;
        }
    }
    for (i=1;i<=clique_num[0];i++)
        if (group[re_order[clique_num[i]]]>0)
        {
            gp[group[re_order[clique_num[i]]]]++;
        } else
            forbid[re_order[clique_num[i]]]=true;

    //maintain next_same array; when we eliminate one vertex, we free (make not forbidden) the next vertex in the corresponding similar group
    freopen("rotate.txt","r",stdin);
    g_count=0;
    while (true)
    {
        scanf("%i",&cluster[0]);
        if (cluster[0]==-1)
            break;
        g_count++;
        for (i=1;i<=cluster[0];i++)
        {
            scanf("%i",&cluster[i]);
            cluster[i]=re_order[cluster[i]];
        }
        for (i=1;i<=gp[g_count];i++)
            forbid[cluster[i]]=true;
        for (i=gp[g_count]+2;i<=cluster[0];i++)
        {
            not_allow[0][cluster[i]]=true;
            next_same[0][cluster[i-1]]=cluster[i];
        }
    }

    int (*g_tmp)[maxn]=(int ((*)[maxn]))malloc(sizeof(int)*maxn*(n+1));
    //change order
    for (i=0;i<n;i++)
    {
        for (j=1;j<=g[0][i][0];j++)
            g_tmp[re_order[i]][j]=re_order[g[0][i][j]];
        g_tmp[re_order[i]][0]=g[0][i][0];
    }
    for (i=n-1;i>=0;i--)
        if (g[0][i][0]>1) break;

    //pre-processing the graph
    if (n-1!=i)
    {
        printf("pre n=%i\n",n);
        n=i+1;
        printf("n changed to %i\n",n);
    }
    printf("n=%i m=%i m/n=%lf\n",n,m,((double)m)/n);


    memcpy(g[0],g_tmp,sizeof(int)*maxn*(n+1));
    free(g_tmp);
    memset(map_hash[0],0,sizeof(map_hash[0]));
    for (i=0;i<n;i++)
        for (j=1;j<=g[0][i][0];j++)
            map_hash[0][i][g[0][i][j]]=j;

    memset(state,0,sizeof(state));
    op[0][0]=n;
    for (i=1;i<=n;i++)
    {
        op[0][i]=i-1;
        hash[0][i-1]=i;
    }

    //ex is the exist-array, which checks duplicates
    free_p=2;
    ex=(link*)malloc(sizeof(link)*maxql);
    ex[1].link_next=0;
    ex[1].lb=mmw(0);
    memset(ex[1].state,0,sizeof(ex[1].state));
    entering=false;


    InitializeCriticalSection(&ub_mutex);
    InitializeCriticalSection(&thread_mutex);
    InitializeCriticalSection(&h_mutex);
    end_signal=CreateSemaphore(NULL,0,Thread_s,"end_signal");

    for (i=0;i<Thread_s;i++)
    {
        pool[i].thread_id=i;
        pool[i].next_thread=i+1;
    }
    //hash_num is used in the hash function
    for (i=0;i<max_bits;i++)
        hash_num[i]=rand()%2000+1;
    pool[Thread_s-1].next_thread=0;
    
    free_thread=1;
    free_thread_left=Thread_s-1;
    waiting_list[0][0]=0;

    lb[0]=ex[1].lb;
    printf("lb=%i\n",lb[0]);

    table=(int*)malloc(sizeof(int)*(h+1));
    memset(table,0,sizeof(int)*(h+1));

    //start search
    CloseHandle(CreateThread(NULL,0,begin_thread,&pool[0].thread_id,0,NULL));

    WaitForSingleObject(end_signal,INFINITE);

    //print information
    printf("treewidth=%i\n",ub);
    long t2=GetTickCount();
    printf("Time used: %.3lfs\n",((double)(t2-t1))/1000);
    printf("Free_p=%i\n",free_p);
    printf("update_time=%i\n",update_thread_time);
    if (!entering)
        printf("standard mode.\n");
    else 
        printf("trick mode.\n");
    free(table);
    free(ex);
    freopen("dfs_output.txt","w",stdout);
    printf("%i\n",ub);
    printf("%li\n",t2-t1);
    printf("%i\n",free_p);
    return 0;
}

