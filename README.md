# ulisp
Micro lisp implementation for study.

## How to build
```
$ make ulisp
```

## How to execute
```
$ ./ulisp
```

## Special forms
* quote ... quote symbol
* cons ... construct pair
* atom ... predicate to check if next s-expression is atom or not (i.e. pair)
* car ... return the 1st expression of pair
* cdr ... return the 2nd expression of pair
* cond ... conditional construct. syntax: (cond (__pred1__ __conseq1__) [(__pred2__ __conseq2__) ...])
* set ... set variable to current environment
* lambda ... construct anonymous function. symtax: (lambda (__params__) __body1__ [__body2__ ...])

There is no value, so you CAN NOT use number nor string.

## Example
```
$ ./ulisp
> (quote ulisp)
ulisp
> t
True
> (set (quote hello) (quote ulisp))
ulisp
> hello
ulisp
> (set (quote r) (lambda (x y) (cond ((atom x) y) (t (r (cdr x) (cons (car x) y))))))
(*applicable* (x y) (cond ((atom x) y) (t (r (cdr x) (cons (car x) y)))))
> (r (quote (a b c d e f)) ())
(f e d c b a)
> 
```
You can quit REPL with `Ctrl+D`.

## Verbose evaluation
If you set `*verbose-eval*` non-nil value, each `eval` prints its evaluation process.

```
$ ./ulisp
> (set (quote *verbose-eval*) t)
True
> (set (quote map) (lambda (f xs) (cond
    ((atom xs) xs)
    ((quote else) (cons (f (car xs)) (map f (cdr xs)))))))
EVALUATE: (set (quote map) (lambda (f xs) (cond ((atom xs) xs) ((quote else) (cons (f (car xs)) (map f (cdr xs)))))))
|  env.*verbose-eval*=True
|  env.map=*applicable*
|  env.t=True
| EVALUATE: (quote map)
| |  env.*verbose-eval*=True
| |  env.map=*applicable*
| |  env.t=True
| \___ map
| EVALUATE: (lambda (f xs) (cond ((atom xs) xs) ((quote else) (cons (f (car xs)) (map f (cdr xs))))))
| |  env.*verbose-eval*=True
| |  env.map=*applicable*
| |  env.t=True
| \___ *applicable*
\___ *applicable*
*applicable*
> 
```

You can trun off this verbose print to set nil to `*verbose-eval*`.
```
> (set (quote *verbose-eval*) ()))
```

## Acknowledgement

This work inspired heavily [小さな Lisp インタープリタ](https://qiita.com/hatsugai/items/ce176446846667b11315).
