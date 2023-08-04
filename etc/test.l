(define fibc (lambda ( n ) (ifElse (< n 2) n (+ (fibc (- n 1)) (fibc (- n 2))))))
( fibc 30 )
(define nfibs
  (lambda (n)
    (ifElse (< n 2)
	n
      (+ (nfibs (- n 1)) (nfibs (- n 2))))))
(nfibs 30)