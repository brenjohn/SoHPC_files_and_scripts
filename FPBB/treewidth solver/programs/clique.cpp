/*
 *  Author : Yang Yuan  
 *           Department of Computer Science
 *           Peking University
 *
 *  This is a program for computing clique in the graph. Since the graph we use to compute treewidth is usually small, we use simple DFS algorithm here.
 *  It is sufficient for current benchmarks. For better algorithm, please google some delicated algorithms for Maximum Clique.
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
#define maxn 1000
using namespace std;
int g[maxn][maxn];
int s_clique[maxn];
int max_cli;
bool in_clique[maxn];
int clique_ans[maxn];
int n,m;
int map_hash[maxn][maxn];
int dep;
int save[maxn][maxn][2];

void find_c(int cur_cli,int dep)
{
    int i;
    if (cur_cli+s_clique[0]<=max_cli)
        return;
    if (s_clique[0]==0)
    {
        max_cli=cur_cli;
        clique_ans[0]=0;
        for (i=0;i<n;i++)
            if (in_clique[i])
            {
                clique_ans[0]++;
                clique_ans[clique_ans[0]]=i;
            }
        return;
    }
    int save_l,which;
    which=s_clique[1];
    save[dep][0][0]=1;
    save[dep][0][1]=which;
    s_clique[1]=s_clique[s_clique[0]];
    s_clique[0]--;
    find_c(cur_cli,dep+1);
    i=1;
    save_l=1;
    while (i<=s_clique[0])
        if (!map_hash[which][s_clique[i]])
        {
            save[dep][save_l][0]=i;
            save[dep][save_l][1]=s_clique[i];
            save_l++;
            s_clique[i]=s_clique[s_clique[0]];
            s_clique[0]--;
        } else i++;
    in_clique[which]=true;
    find_c(cur_cli+1,dep+1);
    in_clique[which]=false;
    for (i=save_l-1;i>=0;i--)
    {
        s_clique[0]++;
        s_clique[s_clique[0]]=s_clique[save[dep][i][0]];
        s_clique[save[dep][i][0]]=save[dep][i][1];
    }
}

int main(int argc,char * argv[])
{
    ifstream fin(argv[1]);
    string s;
    int i,j;
    memset(map_hash,0,sizeof(map_hash));
    while (getline(fin,s))
    {
        if (s[0]=='p')
            sscanf(s.c_str(),"%*[^0-9]%i%i",&n,&m);
        if (s[0]=='e')
        {
            sscanf(s.c_str(),"%*[^0-9]%i%i",&i,&j);
            i--;
            j--;
            if (map_hash[i][j]==0)
            {
                g[i][0]++;
                g[i][g[i][0]]=j;
                g[j][0]++;
                g[j][g[j][0]]=i;
                map_hash[i][j]=g[i][0];
                map_hash[j][i]=g[j][0];
            }
        }
    }
    memset(map_hash,0,sizeof(map_hash));
    for (i=0;i<n;i++)
        for (j=1;j<=g[i][0];j++)
            map_hash[i][g[i][j]]=j;
    max_cli=0;
    s_clique[0]=n;
    for (i=1;i<=n;i++)
        s_clique[i]=i;
    fin.close();
    long t1=GetTickCount();
    find_c(0,0);
    long t2=GetTickCount();
    printf("clique size: %i\n",max_cli);
    freopen("clique.txt","w",stdout);
    printf("%li\n",t2-t1);
    printf("%i\n",max_cli);
    for (i=1;i<=clique_ans[0];i++)
        printf("%i  ",clique_ans[i]);
    return 0;
}
