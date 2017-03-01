template<int N>   //里面的是参数列表
class Kcoherence
{
public:
  unsigned _idx[N];   //index，unsigned  范围更大一些 
  float _err[N];
  int n;

  Kcoherence() { n=0; }
  void insert(unsigned idx, float err) {       
//前面那个是返回值的类型返回类型：一个函数可以返回一个值
// error is already greater than any member
    if(n==N && err > _err[n-1])   
//1.当 err d大于；n=N的时候，为1，；两边都成立的时候，就成立了
//2. 看
      return;
    if(n<N) n++;
    _idx[n-1] = idx;
    _err[n-1] = err;
    // bubble up to maintain sorted list
    // i guess this could be a heap but... that'd be a waste
    for(int i=n-2;i>=0;i--) {
      if(_err[i] > _err[i+1]) {
        float t = _err[i]; _err[i] = _err[i+1]; _err[i+1] = t;
        unsigned te = _idx[i]; _idx[i] = _idx[i+1]; _idx[i+1] = te;
      }
    }
#if 0
    printf("insert: ");
    for(int i=0;i<n;i++)
      printf("%u(%u) ", _idx[i], _err[i]);
    printf("\n");
#endif
  }

  unsigned operator[](int n) const { return _idx[n]; }  //这是什么，一直比较到最前那个一个
};

//：将数列中的数字从大到小进行排列，顾名思义就是将一个数从下面浮上来。其实，就是一个for循环，最外面的循环控制循环的次数，需要有n-1次循环。