#ifndef COMBO_H_
#define COMBO_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef int           boolean; /* logical variable         */
typedef int           ntype;   /* number of states/items   */
typedef long          itype;   /* item profits and weights */
typedef long          stype;   /* sum of profit or weight  */
typedef unsigned long btype;   /* binary solution vector   */
typedef double        prod;    /* product of state, item   */

typedef int (*funcptr) (const void *, const void *);

/* item record */
typedef struct {
  itype   p;              /* profit                  */
  itype   w;              /* weight                  */
  boolean x;              /* solution variable       */
    int index;
} item;

stype combo(item *f, item *l, stype c, stype lb, stype ub,
            boolean def, boolean relx);

#ifdef __cplusplus
}
#endif

#endif
