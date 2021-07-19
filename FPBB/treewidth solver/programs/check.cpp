/*
 *  Author : Yang Yuan  
 *           Department of Computer Science
 *           Peking University
 *
 *  This is a program for checking the answer of computing treewidth.
 *  The algorithm is using the elimination ordering in the answer file, to compute the treewidth according to the definition.
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
#define maxn 1000
using namespace std;

int g[maxn][maxn];
bool map_hash[maxn][maxn];

int main(int argc, char* argv[])
{
    char *f=new char[100];
    sprintf(f,"%s.txt",argv[1]);

    char* filename=new char[100];
    string s;
    ifstream fin;

    fin.open(f);
    getline(fin,s);
    sprintf(filename,"%s",s.c_str());
    fin.close();
    int n,m,i,j;
    
    fin.open(filename);
    memset(map_hash,0,sizeof(map_hash));
    bool get[maxn];
    memset(get,0,sizeof(get));
    n=0;
    while (getline(fin,s))
    {
        if (s[0]=='e')
        {
            sscanf(s.c_str(),"%*[^0-9]%i%i",&i,&j);
            if (!get[i])
            {
                get[i]=true;
                n++;
            }
            if (!get[j])
            {
                get[j]=true;
                n++;
            }
            if (!map_hash[i][j])
            {
                m++;
                g[i][0]++;
                g[i][g[i][0]]=j;
                g[j][0]++;
                g[j][g[j][0]]=i;
                map_hash[i][j]=true;
                map_hash[j][i]=true;
            }
        }
    }
    fin.close();
    printf("%s\n",f);
    freopen(f,"r",stdin);
    scanf("%s",filename);
    int mini=0,left=n;
    int cur;
    while (scanf("%i",&cur))
    {
        if (cur==-1)
            break;
        left--;
        if (g[cur][0]>mini)
            mini=g[cur][0];
        for (int i=1;i<g[cur][0];i++)
            for (int j=i+1;j<=g[cur][0];j++)
                if (!map_hash[g[cur][i]][g[cur][j]])
                {
                    map_hash[g[cur][i]][g[cur][j]]=true;
                    map_hash[g[cur][j]][g[cur][i]]=true;
                    g[g[cur][i]][0]++;
                    g[g[cur][i]][g[g[cur][i]][0]]=g[cur][j];
                    g[g[cur][j]][0]++;
                    g[g[cur][j]][g[g[cur][j]][0]]=g[cur][i];
                }
        for (int i=1;i<=g[cur][0];i++)
        {
            for (int j=1;j<=g[g[cur][i]][0];j++)
                if (cur==g[g[cur][i]][j])
                {
                    g[g[cur][i]][j]=g[g[cur][i]][g[g[cur][i]][0]];
                    g[g[cur][i]][0]--;
                    break;
                }
        }
    }
    if (mini<left-1)
        mini=left-1;
    printf("treewidth=%i\n",mini);
    fin.close();
    return 0;
}
