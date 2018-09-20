# ulisp
Micro lisp implementation for study.

## How to build
```
$ make -C src ulisp
```

## How to execute
```
$ src/ulisp
```

## Specials
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
$ src/ulisp
> (quote ulisp)
ulisp
> t
True
> (set (quote hello) (quote ulisp))
ulisp
> hello
ulisp
> 
```
You can quit REPL with `Ctrl+D`.
