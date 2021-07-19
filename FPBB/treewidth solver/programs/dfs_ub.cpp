/*
 *  Author : Yang Yuan  
 *           Department of Computer Science
 *           Peking University
 *
 *  This program is used for computing the upper bound of the treewidth
 *  It uses the algorithm MFPBB, please see the paper 'A Fast Parallel Branch and Bound Algorithm for Treewidth', ICTAI 2011.
 *
 *  For any question about the algorithm or this program, feel free to contact the author: yyuanchn@gmail.com
 *
 *  08/27/2011
 */

#include <stdio.h>
#include <string>
#include <fstream>
#include <iostream>
#include <windows.h>
#include <tchar.h>
#define maxql 31000
#define bits 32
#define h 9999983
#define maxm (maxn*maxn)
#define sum_limit 1000
#define per_choice 3
#define dfs_state 30000
#include "machine.def"

#define maxn 150
#define max_bits 4
#define trick_mode false
using namespace std;
struct link
{
    int link_next;
    int state[max_bits];
    int lb;
};
struct thread_node
{
    int thread_id;
    int next_thread;
};

int n,m,order[maxn],ub;
int forbid[maxn],cluster[maxn];
int re_order[maxn];
int bitcode[bits];
int pos_bit[maxn][2];
bool entering;



int* table;
link* ex;


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
int eli[Thread_s][maxn];
char * filename;
int choice[Thread_s][maxn][maxn];
int g_tmp[Thread_s][maxn][maxn];
int op_tmp[Thread_s][maxn];
int map_hash_tmp[Thread_s][maxn][maxn];

int remove_[Thread_s][maxn][maxn];
int restore[Thread_s][maxn][maxn];
int value[Thread_s][maxn][maxn];
int hash_num[max_bits];
int group[maxn];
int gp[maxn];



HANDLE end_signal;
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


int make_pos(int thread)
{
    int i,s;
    s=0;
    for (i=0;i<max_bits;i++)
        s=(s+(state[thread][i]%h)*hash_num[i])%h;
    return (s+h)%h+1;
}

bool same(int thread,int where)
{
    int i;
    for (i=0;i<max_bits;i++)
        if (ex[where].state[i]!=state[thread][i])
            return false;
    return true;
}

bool add_state(int thread,int dep,int lb)
{
    state[thread][pos_bit[dep][0]]^=pos_bit[dep][1];
    int pos=make_pos(thread);
    int where=table[pos];
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
                state[thread][pos_bit[dep][0]]^=pos_bit[dep][1];
                    ex[where].lb=lb;
                return true;
            }
        } else
            where=ex[where].link_next;
    }

    EnterCriticalSection(&h_mutex);

        memcpy(ex[free_p].state,state[thread],sizeof(ex[free_p].state));
        ex[free_p].lb=lb;
        ex[free_p].link_next=table[pos];
        table[pos]=free_p;
        free_p++;

    LeaveCriticalSection(&h_mutex);

    if (free_p>=maxql)
        printf("overflow!!\n");
        
    state[thread][pos_bit[dep][0]]^=pos_bit[dep][1];
    return true;
}

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
int mmw(int thread,int which)
{
    memcpy(op_tmp[thread],op[thread],sizeof(op[thread]));
    memcpy(g_tmp[thread],g[thread],sizeof(g[thread]));
    memcpy(map_hash_tmp[thread],map_hash[thread],sizeof(map_hash[thread]));

    int ans;
    int i,j,k,remove[maxn];
    int dep=op_tmp[thread][which];
    op_tmp[thread][0]--;
    remove[0]=0;
    for (i=1;i<=g_tmp[thread][dep][0];i++)
    {
        remove[0]++;
        remove[remove[0]]=g_tmp[thread][dep][i];
    }
    for (i=1;i<remove[0];i++)
        for (j=i+1;j<=remove[0];j++)
            if (map_hash_tmp[thread][remove[i]][remove[j]]==0)
            {
                g_tmp[thread][remove[i]][0]++;
                g_tmp[thread][remove[i]][g_tmp[thread][remove[i]][0]]=remove[j];
                map_hash_tmp[thread][remove[i]][remove[j]]=g_tmp[thread][remove[i]][0];
                g_tmp[thread][remove[j]][0]++;
                g_tmp[thread][remove[j]][g_tmp[thread][remove[j]][0]]=remove[i];
                map_hash_tmp[thread][remove[j]][remove[i]]=g_tmp[thread][remove[j]][0];
            }
    for (i=1;i<=remove[0];i++)
    {
        g_tmp[thread][remove[i]][map_hash_tmp[thread][remove[i]][dep]]=g_tmp[thread][remove[i]][g_tmp[thread][remove[i]][0]];
        g_tmp[thread][remove[i]][0]--;
        map_hash_tmp[thread][remove[i]][g_tmp[thread][remove[i]][map_hash_tmp[thread][remove[i]][dep]]]=map_hash_tmp[thread][remove[i]][dep];
        map_hash_tmp[thread][remove[i]][dep]=0;
    }
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
    return ans;
}


void update_ub(int new_ub,int thread)
{
    EnterCriticalSection(&ub_mutex);
        if (ub>new_ub)
        {
            ub=new_ub;
            FILE *fp;
            fp=fopen("rlt.txt","w");
            fprintf(fp,"%s\n",filename);
            for (int i=n;i>op[thread][0];i--)
                fprintf(fp,"%i\n",order[eli[thread][i]]);
            fprintf(fp,"-1\n");
            fclose(fp);
        }
    LeaveCriticalSection(&ub_mutex);
    printf("new ub=%i  ",ub);
    printf("free_p=%i  ",free_p);
    long t2=GetTickCount();
    printf("Time used: %.3lfs\n",((double)(t2-t1))/1000);
}

int cal_state(int thread,int node)
{
    int aa=mmw(thread,node);
    int i,j,s=0;
    for (i=1;i<=op[thread][0];i++)
        s+=g[thread][op[thread][i]][0];
    s/=2;
    s-=g[thread][node][0];
    for (i=1;i<g[thread][node][0];i++)
        for (j=i+1;j<=g[thread][node][0];j++)
            if (map_hash[thread][g[thread][node][i]][g[thread][node][j]]==0)
                s++;
    return aa*maxm+s;
}

DWORD WINAPI begin_thread(LPVOID param);
void dfs(int thread,int forced,int sum,int lowerb) 
{
    if (free_p>dfs_state)
        return;
    int i;
    //int value[maxn];
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
        if (lowerb<ub)
        {
            if (!entering)
            {
                int sim_rlt=simplicial(thread,lowerb);
                if (sim_rlt==-1)
                {
                    if (trick_mode)
                        entering=true;
                    for (i=1;i<=op[thread][0];i++)
                        if ((!forbid[op[thread][i]]) && (g[thread][op[thread][i]][0]<ub)&& (!not_allow[thread][op[thread][i]]))
                            if (add_state(thread,op[thread][i],max(lowerb,g[thread][op[thread][i]][0])))
                            {
                                choice[thread][label][0]++;
                                choice[thread][label][choice[thread][label][0]]=op[thread][i];
                                value[thread][label][choice[thread][label][0]]=cal_state(thread,i);
                            }
                } else 
                    if (sim_rlt>-1)
                    {
                        choice[thread][label][0]=1;
                        choice[thread][label][1]=sim_rlt;
                    }
            } else
            {
                for (i=1;i<=op[thread][0];i++)
                    if ((!forbid[op[thread][i]]) && (g[thread][op[thread][i]][0]<ub)&& (!not_allow[thread][op[thread][i]]))
                        if (add_state(thread,op[thread][i],max(lowerb,g[thread][op[thread][i]][0])))
                        {
                            choice[thread][label][0]++;
                            choice[thread][label][choice[thread][label][0]]=op[thread][i];
                            value[thread][label][choice[thread][label][0]]=cal_state(thread,i);
                        }
            }
        }
    } else
    {
        choice[thread][label][0]=1;
        choice[thread][label][1]=forced;
    }
    if (choice[thread][label][0]==0) return;
    int j,k;
    for (i=1;i<choice[thread][label][0];i++)
        for (j=i+1;j<=choice[thread][label][0];j++)
            if (value[thread][label][i]>value[thread][label][j])
            {
                k=value[thread][label][i];
                value[thread][label][i]=value[thread][label][j];
                value[thread][label][j]=k;
                k=choice[thread][label][i];
                choice[thread][label][i]=choice[thread][label][j];
                choice[thread][label][j]=k;
            }
    if ((choice[thread][label][0]>1) && (sum<sum_limit) && (free_thread_left>0))
    {
        EnterCriticalSection(&thread_mutex);

            if (free_thread_left>0)
            {
                int use_thread=free_thread_left;
                if (use_thread>choice[thread][label][0]-1)
                    use_thread=choice[thread][label][0]-1;
                use_thread++;
                int cur_pos=1,cur_len=1;
                for (int i=1;i<use_thread;i++)
                {
                    cur_pos+=cur_len;

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
                choice[thread][label][0]=1;
            }

        LeaveCriticalSection(&thread_mutex);
    }
    if (sum<sum_limit) sum*=choice[thread][label][0];
    if (choice[thread][label][0]>per_choice)
        choice[thread][label][0]=per_choice;

    int op_last,interv[2],dep,cur_choice;
    //int remove[maxn],restore[maxn];
    for (cur_choice=1;cur_choice<=choice[thread][label][0];cur_choice++)
    {
        dep=choice[thread][label][cur_choice];
        eli[thread][label]=dep;

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


        dfs(thread,-1,sum,max(lowerb,g[thread][dep][0]));

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

int main(int argc, char* argv[])
{
    filename=new char[100];
    sprintf(filename,"instances\\%s.col",argv[1]);
    //printf("begin!\n");
    ub=atoi(argv[2]);


    freopen("clique.txt","r",stdin);
    int clique_time;
    scanf("%i",&clique_time);
    int clique_num[maxn];
    scanf("%i",&clique_num[0]);
    int i,j,k;
    for (i=1;i<=clique_num[0];i++)
        scanf("%i",&clique_num[i]);

    t1=GetTickCount();
    ifstream fin;
    string s;
    fin.open(filename);
    bitcode[0]=1;
    for (i=1;i<bits;i++)
        bitcode[i]=bitcode[i-1]*2;
    while (getline(fin,s))
    {
        if (s[0]=='p')
            sscanf(s.c_str(),"%*[^0-9]%i%i",&n,&m);
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

    for (i=0;i<n;i++)
    {
        for (j=1;j<=g[0][i][0];j++)
            g_tmp[0][re_order[i]][j]=re_order[g[0][i][j]];
        g_tmp[0][re_order[i]][0]=g[0][i][0];
    }
    for (i=n-1;i>=0;i--)
        if (g[0][i][0]>1) break;

    if (n-1!=i)
    {
        printf("pre n=%i\n",n);
        n=i+1;
        printf("n changed to %i\n",n);
    }

    memcpy(g[0],g_tmp[0],sizeof(int)*maxn*(n+1));
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

    free_p=2;
    ex=(link*)malloc(sizeof(link)*maxql);
    ex[1].lb=mmw(0,0);
    ex[1].link_next=0;
    memset(ex[1].state,0,sizeof(ex[1].state));
    entering=false;

    InitializeCriticalSection(&ub_mutex);
    InitializeCriticalSection(&thread_mutex);
    InitializeCriticalSection(&h_mutex);
    end_signal=CreateSemaphore(NULL,0,Thread_s,"end_signal");
    if (ub==-1)
        ub=n-1;

    for (i=0;i<Thread_s;i++)
    {
        pool[i].thread_id=i;
        pool[i].next_thread=i+1;
    }
    pool[Thread_s-1].next_thread=0;
    free_thread=1;
    free_thread_left=Thread_s-1;
    waiting_list[0][0]=0;
    lb[0]=ex[1].lb;
    table=(int*)malloc(sizeof(int)*(h+1));
    memset(table,0,sizeof(int)*(h+1));
    for (i=0;i<max_bits;i++)
        hash_num[i]=rand()%2000+1;

    CloseHandle(CreateThread(NULL,0,begin_thread,&pool[0].thread_id,0,NULL));

    WaitForSingleObject(end_signal,INFINITE);
    //printf("treewidth=%i\n",ub);
    //long t2=GetTickCount();
    //printf("Time used: %.3lfs\n",((double)(t2-t1))/1000);
    //printf("Free_p=%i\n",free_p);
    freopen("dfs_ub_output.txt","w",stdout);
    printf("%i\n",ub);
    free(table);
    free(ex);
    return 0;
}
