
// argc - r0
// argv - r1
// envp - r2
_start:
	mov	fp, #0

	ldr	r12, =environ
	str	r2, [r12]

	bl	main
	mov r0, 0
	bl  exit	
