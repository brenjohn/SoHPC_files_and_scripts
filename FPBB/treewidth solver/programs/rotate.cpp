/*
 *  Author : Yang Yuan  
 *           Department of Computer Science
 *           Peking University
 *
 *  This is a program for finding similar groups in the graph. It uses disjoint sets. 
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
#define h 999983
using namespace std;
int g[maxn][maxn];
int n,m;
int map_hash[maxn][maxn];
int father[maxn];


bool yes(int i,int j)
{
    int k;
    for (k=1;k<=g[i][0];k++)
        if (g[i][k]!=j)
            if (!map_hash[j][g[i][k]])
                return false;
    return true;
}
int findfather(int cur)
{
    if (father[cur]!=cur)
        father[cur]=findfather(father[cur]);
    return father[cur];
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
    fin.close();
    int rank[maxn];
    for (i=0;i<n;i++)
    {
        father[i]=i;
        rank[i]=1;
    }
    long t1=GetTickCount();
    for (i=0;i<n-1;i++)
        for (j=i+1;j<n;j++)
            if ((g[i][0]>1) && (g[i][0]==g[j][0]))
                if (yes(i,j))
                {
                    father[i]=findfather(i);
                    father[j]=findfather(j);
                    if (father[i]!=father[j])
                    {
                        father[father[i]]=father[j];
                        rank[father[j]]+=rank[father[i]];
                    }
                }
    long t2=GetTickCount();
    for (i=0;i<n;i++)
        father[i]=findfather(i);
    int groups=0,involved=0;
    for (i=0;i<n;i++)
        if ((father[i]==i) && (rank[i]>1))
        {
            groups++;
            involved+=rank[i];
        }
    printf("groups=%i involved=%i\n",groups,involved);
    freopen("rotate.txt","w",stdout);
    for (i=0;i<n;i++)
        if ((father[i]==i) && (rank[i]>1))
        {
            printf("%i ",rank[i]);
            for (j=0;j<n;j++)
                if (father[j]==i)
                    printf("%i ",j);
            printf("\n");
        }
    printf("-1\n");
    printf("%li\n",t2-t1);
    return 0;
}
