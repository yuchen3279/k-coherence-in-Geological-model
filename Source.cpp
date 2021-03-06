#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "Grid.h"
#include "kcoherence.h"

// maximum generator iterations
#define ITERATIONS 16
// k-coherence search size
#define _K_ 6
// number of similar versions to generate
#define VERSIONS 8

// neighborhood difference
// Nsize is the radius of pixels surrounding the source pixel.  
// e.g. Nsize=2:
// N N N N N
// N N N N N
// N N p N N
// N N N N N
// N N N N N
unsigned neighbor_diff(Grid *im1, Grid *im2, int x1, int y1, int x2, int y2, int Nsize)
{
  unsigned diff=0;
  int j1 = im1->wrapw(y1-Nsize);
  int j2 = im2->wrapw(y2-Nsize);
  int i1 = im1->wrapl(x1-Nsize);
  int i2 = im2->wrapl(x2-Nsize);
  int ywrap1 = im1->h - j1;
  int ywrap2 = im2->h - j2;
  for(int j=Nsize*2+1;j--;) {
    int idx1 = i1 + j1*im1->w;
    int idx2 = i2 + j2*im2->w;
    int xwrap1 = im1->w - i1;
    int xwrap2 = im2->w - i2;
    //printf("xy1=%d,%d xy2=%d,%d ij1=%d,%d ij2=%d,%d\n", x1,y1, x2,y2, i1,j2, i2,j2);
    for(int i=Nsize*2+1;i--;) {
      //printf("idx1=%d idx2=%d\n", idx1, idx2);
      diff += Grid::normsqr(im1->p(idx1++), im2->p(idx2++));
      xwrap1--; if(!xwrap1) { idx1 -= im1->w; }
      xwrap2--; if(!xwrap2) { idx2 -= im2->w; }
    }
    j1++; j2++;
    ywrap1--; if(!ywrap1) { j1 -= im1->h; }
    ywrap2--; if(!ywrap2) { j2 -= im2->h; }
  }
  return diff;
}

#if 0
// find nearest neighborhood in srcim corresponding to a given block in dstim
// returns an index
unsigned nn_search(Grid *srcim, Grid *dstim, int di, int dj, int Nsize, bool srcwrap, unsigned &e)
{
  unsigned bestdiff = ~0;
  unsigned bestidx = 0;
  int inset = srcwrap ? 0 : Nsize;
  for(int y=inset; y < srcim->h - inset; y++) {
    for(int x=inset; x < srcim->w - inset; x++) {
      unsigned diff = neighbor_diff(srcim, dstim, x,y, di,dj, Nsize);
      if(diff < bestdiff) {
        bestdiff = diff;
        bestidx = srcim->ij_to_idx(x,y);
      }
    }
  }
  e = bestdiff;
  return bestidx;
}
#endif


// find nearest
Kcoherence<_K_> nn_search(Grid *srcim, int si, int sj, int Nsize, bool srcwrap)
{
  int inset = srcwrap ? 0 : Nsize;
  Kcoherence<_K_> best;
  for(int y=inset; y < srcim->h - inset; y++) {
    for(int x=inset; x < srcim->w - inset; x++) {
      //if(x == si && y == sj)
      //  continue;
      unsigned diff = neighbor_diff(srcim, srcim, x,y, si,sj, Nsize);
//      printf("sidx %d didx %d diff = %u\n", srcim->ij_to_idx(si,sj), srcim->ij_to_idx(x,y), diff);
      best.insert(srcim->ij_to_idx(x,y), diff);
    }
  }
  return best;
}

template<int K>
class TextureSynth
{
public:
  Grid *srcim, *dstim;
  Kcoherence<K> *srck;
  Grid *dstz, *dsto; // z array in M step, and origin of current pixel in E step
  bool srcwrap;
  int Nsize;

  TextureSynth(Grid *_src, int dstx, int dsty, int _Nsize, bool _srcwrap) {
    srcim = _src;
    dstim = new Grid(dstx, dsty);
    dstz = new Grid(dstx, dsty);
    dsto = new Grid(dstx, dsty);
    srcwrap = _srcwrap;
    Nsize = _Nsize;
    analyze_srcGrid();
  }

  void analyze_srcGrid()
  {
    printf("analyzing source Grid..."); fflush(stdout);
    srck = new Kcoherence<K>[srcim->w*srcim->h];
    int inset = srcwrap ? 0 : Nsize;
    for(int j=inset;j<srcim->h-inset;j++)
      for(int i=inset;i<srcim->w-inset;i++) {
        int idx = srcim->ij_to_idx(i,j);
        srck[idx] = ::nn_search(srcim, i,j, Nsize, srcwrap);
#if 0
        printf("%d: ", idx);
        for(int k=0;k<srck[idx].n;k++) {
          printf("%d(%d)%s", srck[idx]._idx[k], srck[idx]._err[k], k==K-1?"":",");
        }
        printf("\n");
#endif
      }
    printf("done\n");
  }

  void init(int offset) {
    for(int j=offset;j<dstim->h;j++)
      for(int i=offset;i<dstim->w;i++) {
        int x = rand()%srcim->w;
        int y = rand()%srcim->h;
        dstz->p(i,j) = srcim->ij_to_idx(x,y);
        x = rand()%srcim->w;
        y = rand()%srcim->h;
        //dsto->p(i,j) = srcim->ij_to_idx(x,y);
        dstim->p(i,j) = srcim->p(x,y);
      }
  }

  // nearest neighbor search, using a given coherence set
  void nn_search(Grid *destim, int di, int dj, const Kcoherence<K> &kset,
                 int offx, int offy,
                 unsigned &bestdiff,unsigned &bestidx)
  {
    for(int k=0;k<kset.n;k++) {
      int i,j;
      srcim->idx_to_ij(kset[k],i,j);
      i = srcim->wrapw(i+offx);
      j = srcim->wraph(j+offy);
      if(!srcwrap && (i<Nsize || i>=srcim->w-Nsize))
        continue;
      if(!srcwrap && (j<Nsize || j>=srcim->h-Nsize))
        continue;
      unsigned diff = neighbor_diff(destim, srcim, di,dj, i,j, Nsize);
      if(diff < bestdiff) {
        bestdiff = diff;
        bestidx = srcim->ij_to_idx(i,j);
      }
    }
  }

  unsigned estep(int offset) {
    unsigned E=0;
    for(int j=offset;j<dstim->h;j++)
      for(int i=offset;i<dstim->w;i++) {
        unsigned bestdiff = ~0, bestidx = 0;

        // x,y are the target patch (in the z array)
        //int x,y; srcim->idx_to_ij(dstz->p(i,j),x,y);

        // we search our candidate set k from the pixel's current source
        nn_search(dstim, i,j, srck[dstz->p(i,j)], 0,0, bestdiff, bestidx);
        E += bestdiff;
        dstim->p(i,j) = srcim->p(bestidx);
        dstz->p(i,j) = bestidx;
      }
    return E;
  }

  unsigned mstep(int offset) {
    unsigned E=0;
    bool changed = false;
    for(int j=offset;j<dstim->h;j++)
      for(int i=offset;i<dstim->w;i++) {
        // search k set of all neighborhood pixels to find best matching
        // neighborhood between src and dest(i,j)
        unsigned bestdiff = ~0, bestidx = 0;
        for(int nj=-Nsize;nj<=Nsize;nj++)
          for(int ni=-Nsize;ni<=Nsize;ni++) {
            int x=dstim->wrapw(i+ni), y=dstim->wraph(j+nj);
            nn_search(dstim, i,j, srck[dstz->p(x,y)], -ni,-nj, bestdiff, bestidx);
          }

        E += bestdiff;
        if(dstz->p(i,j) != bestidx) {
          changed = true;
          dstz->p(i,j) = bestidx;
        }
        dstim->p(i,j) = srcim->p(bestidx);
      }
    if(!changed)
      return 0;
    return E;
  }

  ~TextureSynth() { delete srcim; delete dstim; }
};

int main(int argc, char **argv)
{
  if(argc < 6) {
    printf("usage: %s <source image> <srcwrap> <neighborhood> <destw> <desth>\n", argv[0]);
    return -1;
  }
  srand(time(NULL));

  Grid *srcim = Grid::load_png(argv[1]);
  printf("loaded %dx%d source\n", srcim->w, srcim->h);

  int w = atoi(argv[4]);
  int h = atoi(argv[5]);
  TextureSynth<_K_> synth(srcim, w, h, atoi(argv[3]), atoi(argv[2])?true:false);
  printf("synthesizing %dx%d with neighborhood=%d (%d^2), srcwrap=%s\n", w,h,
         synth.Nsize, synth.Nsize*2+1,
         synth.srcwrap ? "on" : "off");
  int keeprows = (synth.Nsize+3)/2;

  for(int version=0;version<VERSIONS;version++) {
    synth.init(version==0 ? 0 : synth.Nsize);
    for(int iterations=0;iterations<ITERATIONS;iterations++) {
      printf("\rout%d.png iteration %d/%d: E:", version, 1+iterations, ITERATIONS);
      printf("%u M:", synth.estep(version==0 ? 0 : keeprows));
      unsigned e = synth.mstep(version==0 ? 0 : keeprows);
      printf("%u\e[K", e);
      fflush(stdout);
      if(!e) break;
    }
    printf(" final err: %d", synth.estep(version==0 ? 0 : keeprows));
    char buf[20];
    sprintf(buf, "out%d.png", version);
    synth.dstim->save_png(buf);
    printf(" done\n");

  }
  return 0;
}

