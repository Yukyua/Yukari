; Verified original EXE SHA256 08bd26c2234e31bfe0f8c286480d28c9ed2632e59b5d4723e9180a1064cd7267
; Supplement to saved REA decompilation; original x86 instructions are authoritative for FPU constants.


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  433e50: a1 7c c8 56 00               	movl	0x56c87c, %eax
  433e55: 56                           	pushl	%esi
  433e56: 33 f6                        	xorl	%esi, %esi
  433e58: 33 c9                        	xorl	%ecx, %ecx
  433e5a: 83 f8 0a                     	cmpl	$0xa, %eax
  433e5d: 57                           	pushl	%edi
  433e5e: 8b 7c 24 0c                  	movl	0xc(%esp), %edi
  433e62: 0f 87 88 00 00 00            	ja	0x433ef0 <.text+0x32ef0>
  433e68: ff 24 85 08 3f 43 00         	jmpl	*0x433f08(,%eax,4)
  433e6f: 33 f6                        	xorl	%esi, %esi
  433e71: 33 c9                        	xorl	%ecx, %ecx
  433e73: eb 7b                        	jmp	0x433ef0 <.text+0x32ef0>
  433e75: 33 f6                        	xorl	%esi, %esi
  433e77: 8b 44 24 1c                  	movl	0x1c(%esp), %eax
  433e7b: 2b 05 74 c8 56 00            	subl	0x56c874, %eax
  433e81: 2b 44 24 14                  	subl	0x14(%esp), %eax
  433e85: 99                           	cltd
  433e86: 2b c2                        	subl	%edx, %eax
  433e88: 8b c8                        	movl	%eax, %ecx
  433e8a: 8b 47 10                     	movl	0x10(%edi), %eax
  433e8d: 99                           	cltd
  433e8e: 2b c2                        	subl	%edx, %eax
  433e90: d1 f9                        	sarl	%ecx
  433e92: d1 f8                        	sarl	%eax
  433e94: 2b c8                        	subl	%eax, %ecx
  433e96: eb 58                        	jmp	0x433ef0 <.text+0x32ef0>
  433e98: 8b 4c 24 1c                  	movl	0x1c(%esp), %ecx
  433e9c: 2b 4f 10                     	subl	0x10(%edi), %ecx
  433e9f: 33 f6                        	xorl	%esi, %esi
  433ea1: 2b 0d 74 c8 56 00            	subl	0x56c874, %ecx
  433ea7: 83 c1 05                     	addl	$0x5, %ecx
  433eaa: eb 44                        	jmp	0x433ef0 <.text+0x32ef0>
  433eac: 8b 74 24 18                  	movl	0x18(%esp), %esi
  433eb0: 33 c9                        	xorl	%ecx, %ecx
  433eb2: eb 3c                        	jmp	0x433ef0 <.text+0x32ef0>
  433eb4: 8b 74 24 18                  	movl	0x18(%esp), %esi
  433eb8: eb bd                        	jmp	0x433e77 <.text+0x32e77>
  433eba: 8b 74 24 18                  	movl	0x18(%esp), %esi
  433ebe: eb 23                        	jmp	0x433ee3 <.text+0x32ee3>
  433ec0: 8b 74 24 20                  	movl	0x20(%esp), %esi
  433ec4: 33 c9                        	xorl	%ecx, %ecx
  433ec6: eb 28                        	jmp	0x433ef0 <.text+0x32ef0>
  433ec8: 8b 74 24 24                  	movl	0x24(%esp), %esi
  433ecc: eb 15                        	jmp	0x433ee3 <.text+0x32ee3>
  433ece: 8b 77 0c                     	movl	0xc(%edi), %esi
  433ed1: 33 c9                        	xorl	%ecx, %ecx
  433ed3: f7 de                        	negl	%esi
  433ed5: eb 19                        	jmp	0x433ef0 <.text+0x32ef0>
  433ed7: 8b 77 0c                     	movl	0xc(%edi), %esi
  433eda: f7 de                        	negl	%esi
  433edc: eb 99                        	jmp	0x433e77 <.text+0x32e77>
  433ede: 8b 77 0c                     	movl	0xc(%edi), %esi
  433ee1: f7 de                        	negl	%esi
  433ee3: 8b 4c 24 1c                  	movl	0x1c(%esp), %ecx
  433ee7: 2b 4f 10                     	subl	0x10(%edi), %ecx
  433eea: 2b 0d 74 c8 56 00            	subl	0x56c874, %ecx
  433ef0: a1 80 c8 56 00               	movl	0x56c880, %eax
  433ef5: 03 c6                        	addl	%esi, %eax
  433ef7: 89 47 04                     	movl	%eax, 0x4(%edi)
  433efa: 8b 15 84 c8 56 00            	movl	0x56c884, %edx
  433f00: 03 d1                        	addl	%ecx, %edx
  433f02: 89 57 08                     	movl	%edx, 0x8(%edi)
  433f05: 5f                           	popl	%edi
  433f06: 5e                           	popl	%esi
  433f07: c3                           	retl
  433f08: 6f                           	outsl	(%esi), %dx
  433f09: 3e 43                        	incl	%ebx
  433f0b: 00 75 3e                     	addb	%dh, 0x3e(%ebp)
  433f0e: 43                           	incl	%ebx
  433f0f: 00 98 3e 43 00 ac            	addb	%bl, -0x53ffbcc2(%eax)
  433f15: 3e 43                        	incl	%ebx
  433f17: 00 b4 3e 43 00 ba 3e         	addb	%dh, 0x3eba0043(%esi,%edi)
  433f1e: 43                           	incl	%ebx
  433f1f: 00 c0                        	addb	%al, %al
  433f21: 3e 43                        	incl	%ebx
  433f23: 00 c8                        	addb	%cl, %al
  433f25: 3e 43                        	incl	%ebx
  433f27: 00 ce                        	addb	%cl, %dh
  433f29: 3e 43                        	incl	%ebx
  433f2b: 00 d7                        	addb	%dl, %bh
  433f2d: 3e 43                        	incl	%ebx
  433f2f: 00 de                        	addb	%bl, %dh
  433f31: 3e 43                        	incl	%ebx
  433f33: 00 cc                        	addb	%cl, %ah
  433f35: cc                           	int3
  433f36: cc                           	int3
  433f37: cc                           	int3
  433f38: cc                           	int3
  433f39: cc                           	int3
  433f3a: cc                           	int3
  433f3b: cc                           	int3
  433f3c: cc                           	int3
  433f3d: cc                           	int3
  433f3e: cc                           	int3
  433f3f: cc                           	int3


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  43fa10: 6a ff                        	pushl	$-0x1
  43fa12: 68 eb 1e 4a 00               	pushl	$0x4a1eeb               # imm = 0x4A1EEB
  43fa17: 64 a1 00 00 00 00            	movl	%fs:0x0, %eax
  43fa1d: 50                           	pushl	%eax
  43fa1e: 83 ec 60                     	subl	$0x60, %esp
  43fa21: 53                           	pushl	%ebx
  43fa22: 55                           	pushl	%ebp
  43fa23: 56                           	pushl	%esi
  43fa24: 57                           	pushl	%edi
  43fa25: a1 48 ea 4b 00               	movl	0x4bea48, %eax
  43fa2a: 33 c4                        	xorl	%esp, %eax
  43fa2c: 50                           	pushl	%eax
  43fa2d: 8d 44 24 74                  	leal	0x74(%esp), %eax
  43fa31: 64 a3 00 00 00 00            	movl	%eax, %fs:0x0
  43fa37: 8b bc 24 84 00 00 00         	movl	0x84(%esp), %edi
  43fa3e: 8b 0d 24 33 4a 00            	movl	0x4a3324, %ecx
  43fa44: 33 ed                        	xorl	%ebp, %ebp
  43fa46: 80 3f 00                     	cmpb	$0x0, (%edi)
  43fa49: 89 6c 24 1c                  	movl	%ebp, 0x1c(%esp)
  43fa4d: 89 6c 24 18                  	movl	%ebp, 0x18(%esp)
  43fa51: 0f 85 a1 05 00 00            	jne	0x43fff8 <.text+0x3eff8>
  43fa57: 39 6f 0c                     	cmpl	%ebp, 0xc(%edi)
  43fa5a: 0f 8e 8e 07 00 00            	jle	0x4401ee <.text+0x3f1ee>
  43fa60: 8b 47 08                     	movl	0x8(%edi), %eax
  43fa63: 33 d2                        	xorl	%edx, %edx
  43fa65: c6 07 01                     	movb	$0x1, (%edi)
  43fa68: 39 28                        	cmpl	%ebp, (%eax)
  43fa6a: 55                           	pushl	%ebp
  43fa6b: 55                           	pushl	%ebp
  43fa6c: 0f 94 c2                     	sete	%dl
  43fa6f: 55                           	pushl	%ebp
  43fa70: 8d 47 10                     	leal	0x10(%edi), %eax
  43fa73: 55                           	pushl	%ebp
  43fa74: 50                           	pushl	%eax
  43fa75: 89 6c 24 3c                  	movl	%ebp, 0x3c(%esp)
  43fa79: c6 44 24 2b 00               	movb	$0x0, 0x2b(%esp)
  43fa7e: 89 54 24 38                  	movl	%edx, 0x38(%esp)
  43fa82: ff d1                        	calll	*%ecx
  43fa84: 8b 84 24 8c 00 00 00         	movl	0x8c(%esp), %eax
  43fa8b: 80 78 68 01                  	cmpb	$0x1, 0x68(%eax)
  43fa8f: 75 05                        	jne	0x43fa96 <.text+0x3ea96>
  43fa91: c6 44 24 17 01               	movb	$0x1, 0x17(%esp)
  43fa96: 8b 47 04                     	movl	0x4(%edi), %eax
  43fa99: 3b c5                        	cmpl	%ebp, %eax
  43fa9b: 74 23                        	je	0x43fac0 <.text+0x3eac0>
  43fa9d: 8b 48 fc                     	movl	-0x4(%eax), %ecx
  43faa0: 8d 70 fc                     	leal	-0x4(%eax), %esi
  43faa3: 68 20 3d 41 00               	pushl	$0x413d20               # imm = 0x413D20
  43faa8: 51                           	pushl	%ecx
  43faa9: 68 98 00 00 00               	pushl	$0x98
  43faae: 50                           	pushl	%eax
  43faaf: e8 39 4e 02 00               	calll	0x4648ed <.text+0x638ed>
  43fab4: 56                           	pushl	%esi
  43fab5: e8 42 48 02 00               	calll	0x4642fc <.text+0x632fc>
  43faba: 83 c4 04                     	addl	$0x4, %esp
  43fabd: 89 6f 04                     	movl	%ebp, 0x4(%edi)
  43fac0: 8b 77 0c                     	movl	0xc(%edi), %esi
  43fac3: 33 c9                        	xorl	%ecx, %ecx
  43fac5: 8b c6                        	movl	%esi, %eax
  43fac7: ba 98 00 00 00               	movl	$0x98, %edx
  43facc: f7 e2                        	mull	%edx
  43face: 0f 90 c1                     	seto	%cl
  43fad1: f7 d9                        	negl	%ecx
  43fad3: 0b c8                        	orl	%eax, %ecx
  43fad5: 33 c0                        	xorl	%eax, %eax
  43fad7: 83 c1 04                     	addl	$0x4, %ecx
  43fada: 0f 92 c0                     	setb	%al
  43fadd: f7 d8                        	negl	%eax
  43fadf: 0b c1                        	orl	%ecx, %eax
  43fae1: 50                           	pushl	%eax
  43fae2: e8 1a 48 02 00               	calll	0x464301 <.text+0x63301>
  43fae7: 83 c4 04                     	addl	$0x4, %esp
  43faea: 89 44 24 30                  	movl	%eax, 0x30(%esp)
  43faee: 3b c5                        	cmpl	%ebp, %eax
  43faf0: 89 6c 24 7c                  	movl	%ebp, 0x7c(%esp)
  43faf4: 74 1d                        	je	0x43fb13 <.text+0x3eb13>
  43faf6: 68 20 3d 41 00               	pushl	$0x413d20               # imm = 0x413D20
  43fafb: 68 30 29 41 00               	pushl	$0x412930               # imm = 0x412930
  43fb00: 56                           	pushl	%esi
  43fb01: 8d 58 04                     	leal	0x4(%eax), %ebx
  43fb04: 68 98 00 00 00               	pushl	$0x98
  43fb09: 53                           	pushl	%ebx
  43fb0a: 89 30                        	movl	%esi, (%eax)
  43fb0c: e8 19 4d 02 00               	calll	0x46482a <.text+0x6382a>
  43fb11: eb 02                        	jmp	0x43fb15 <.text+0x3eb15>
  43fb13: 33 db                        	xorl	%ebx, %ebx
  43fb15: 83 7f 20 00                  	cmpl	$0x0, 0x20(%edi)
  43fb19: 89 5f 04                     	movl	%ebx, 0x4(%edi)
  43fb1c: 8b 9c 24 94 00 00 00         	movl	0x94(%esp), %ebx
  43fb23: c7 44 24 7c ff ff ff ff      	movl	$0xffffffff, 0x7c(%esp) # imm = 0xFFFFFFFF
  43fb2b: 0f 84 1d 01 00 00            	je	0x43fc4e <.text+0x3ec4e>
  43fb31: 8b 8c 24 a4 00 00 00         	movl	0xa4(%esp), %ecx
  43fb38: 51                           	pushl	%ecx
  43fb39: 8b 8c 24 a4 00 00 00         	movl	0xa4(%esp), %ecx
  43fb40: e8 db 89 fd ff               	calll	0x418520 <.text+0x17520>
  43fb45: 8b b4 24 8c 00 00 00         	movl	0x8c(%esp), %esi
  43fb4c: 80 be f8 00 00 00 01         	cmpb	$0x1, 0xf8(%esi)
  43fb53: 89 44 24 20                  	movl	%eax, 0x20(%esp)
  43fb57: 0f 85 ea 00 00 00            	jne	0x43fc47 <.text+0x3ec47>
  43fb5d: 8b 96 f4 00 00 00            	movl	0xf4(%esi), %edx
  43fb63: 8b 4e 14                     	movl	0x14(%esi), %ecx
  43fb66: 52                           	pushl	%edx
  43fb67: e8 e4 2f fd ff               	calll	0x412b50 <.text+0x11b50>
  43fb6c: 85 c0                        	testl	%eax, %eax
  43fb6e: 89 44 24 2c                  	movl	%eax, 0x2c(%esp)
  43fb72: 0f 84 cf 00 00 00            	je	0x43fc47 <.text+0x3ec47>
  43fb78: 83 be fc 00 00 00 00         	cmpl	$0x0, 0xfc(%esi)
  43fb7f: 7c 62                        	jl	0x43fbe3 <.text+0x3ebe3>
  43fb81: 68 98 00 00 00               	pushl	$0x98
  43fb86: e8 e6 3e 02 00               	calll	0x463a71 <.text+0x62a71>
  43fb8b: 8b f0                        	movl	%eax, %esi
  43fb8d: 83 c4 04                     	addl	$0x4, %esp
  43fb90: 85 f6                        	testl	%esi, %esi
  43fb92: 74 12                        	je	0x43fba6 <.text+0x3eba6>
  43fb94: 68 98 00 00 00               	pushl	$0x98
  43fb99: 6a 00                        	pushl	$0x0
  43fb9b: 56                           	pushl	%esi
  43fb9c: e8 6f 47 02 00               	calll	0x464310 <.text+0x63310>
  43fba1: 83 c4 0c                     	addl	$0xc, %esp
  43fba4: eb 02                        	jmp	0x43fba8 <.text+0x3eba8>
  43fba6: 33 f6                        	xorl	%esi, %esi
  43fba8: 8b 84 24 8c 00 00 00         	movl	0x8c(%esp), %eax
  43fbaf: 8b 88 fc 00 00 00            	movl	0xfc(%eax), %ecx
  43fbb5: 8b 54 24 2c                  	movl	0x2c(%esp), %edx
  43fbb9: 6a 00                        	pushl	$0x0
  43fbbb: 6a 00                        	pushl	$0x0
  43fbbd: 6a 00                        	pushl	$0x0
  43fbbf: 6a 00                        	pushl	$0x0
  43fbc1: 51                           	pushl	%ecx
  43fbc2: 52                           	pushl	%edx
  43fbc3: 8b ce                        	movl	%esi, %ecx
  43fbc5: e8 16 aa fc ff               	calll	0x40a5e0 <.text+0x95e0>
  43fbca: 56                           	pushl	%esi
  43fbcb: 8b cb                        	movl	%ebx, %ecx
  43fbcd: e8 ee 31 fd ff               	calll	0x412dc0 <.text+0x11dc0>
  43fbd2: 8b 44 24 20                  	movl	0x20(%esp), %eax
  43fbd6: 89 b0 d4 00 00 00            	movl	%esi, 0xd4(%eax)
  43fbdc: 8b b4 24 8c 00 00 00         	movl	0x8c(%esp), %esi
  43fbe3: 83 be 00 01 00 00 00         	cmpl	$0x0, 0x100(%esi)
  43fbea: 7c 5b                        	jl	0x43fc47 <.text+0x3ec47>
  43fbec: 68 98 00 00 00               	pushl	$0x98
  43fbf1: e8 7b 3e 02 00               	calll	0x463a71 <.text+0x62a71>
  43fbf6: 8b f0                        	movl	%eax, %esi
  43fbf8: 83 c4 04                     	addl	$0x4, %esp
  43fbfb: 85 f6                        	testl	%esi, %esi
  43fbfd: 74 12                        	je	0x43fc11 <.text+0x3ec11>
  43fbff: 68 98 00 00 00               	pushl	$0x98
  43fc04: 6a 00                        	pushl	$0x0
  43fc06: 56                           	pushl	%esi
  43fc07: e8 04 47 02 00               	calll	0x464310 <.text+0x63310>
  43fc0c: 83 c4 0c                     	addl	$0xc, %esp
  43fc0f: eb 02                        	jmp	0x43fc13 <.text+0x3ec13>
  43fc11: 33 f6                        	xorl	%esi, %esi
  43fc13: 8b 8c 24 8c 00 00 00         	movl	0x8c(%esp), %ecx
  43fc1a: 8b 91 00 01 00 00            	movl	0x100(%ecx), %edx
  43fc20: 8b 44 24 2c                  	movl	0x2c(%esp), %eax
  43fc24: 6a 00                        	pushl	$0x0
  43fc26: 6a 00                        	pushl	$0x0
  43fc28: 6a 00                        	pushl	$0x0
  43fc2a: 6a 00                        	pushl	$0x0
  43fc2c: 52                           	pushl	%edx
  43fc2d: 50                           	pushl	%eax
  43fc2e: 8b ce                        	movl	%esi, %ecx
  43fc30: e8 ab a9 fc ff               	calll	0x40a5e0 <.text+0x95e0>
  43fc35: 56                           	pushl	%esi
  43fc36: 8b cb                        	movl	%ebx, %ecx
  43fc38: e8 83 31 fd ff               	calll	0x412dc0 <.text+0x11dc0>
  43fc3d: 8b 4c 24 20                  	movl	0x20(%esp), %ecx
  43fc41: 89 b1 d8 00 00 00            	movl	%esi, 0xd8(%ecx)
  43fc47: 8b 54 24 20                  	movl	0x20(%esp), %edx
  43fc4b: 89 57 24                     	movl	%edx, 0x24(%edi)
  43fc4e: 83 7f 0c 00                  	cmpl	$0x0, 0xc(%edi)
  43fc52: c7 44 24 30 00 00 00 00      	movl	$0x0, 0x30(%esp)
  43fc5a: 0f 8e 81 03 00 00            	jle	0x43ffe1 <.text+0x3efe1>
  43fc60: 33 c0                        	xorl	%eax, %eax
  43fc62: 89 44 24 20                  	movl	%eax, 0x20(%esp)
  43fc66: 89 44 24 2c                  	movl	%eax, 0x2c(%esp)
  43fc6a: 8d 9b 00 00 00 00            	leal	(%ebx), %ebx
  43fc70: 8b 84 24 84 00 00 00         	movl	0x84(%esp), %eax
  43fc77: 8b 70 08                     	movl	0x8(%eax), %esi
  43fc7a: 03 74 24 2c                  	addl	0x2c(%esp), %esi
  43fc7e: 0f 84 35 03 00 00            	je	0x43ffb9 <.text+0x3efb9>
  43fc84: 8b 46 04                     	movl	0x4(%esi), %eax
  43fc87: 83 f8 03                     	cmpl	$0x3, %eax
  43fc8a: 0f 8c dc 00 00 00            	jl	0x43fd6c <.text+0x3ed6c>
  43fc90: 83 f8 05                     	cmpl	$0x5, %eax
  43fc93: 7e 13                        	jle	0x43fca8 <.text+0x3eca8>
  43fc95: 83 f8 07                     	cmpl	$0x7, %eax
  43fc98: 0f 85 ce 00 00 00            	jne	0x43fd6c <.text+0x3ed6c>
  43fc9e: c6 44 24 17 01               	movb	$0x1, 0x17(%esp)
  43fca3: e9 c4 00 00 00               	jmp	0x43fd6c <.text+0x3ed6c>
  43fca8: 8b 06                        	movl	(%esi), %eax
  43fcaa: 39 44 24 24                  	cmpl	%eax, 0x24(%esp)
  43fcae: 0f 84 a5 00 00 00            	je	0x43fd59 <.text+0x3ed59>
  43fcb4: 8b 94 24 8c 00 00 00         	movl	0x8c(%esp), %edx
  43fcbb: 8b c8                        	movl	%eax, %ecx
  43fcbd: 8a 42 68                     	movb	0x68(%edx), %al
  43fcc0: 3c 01                        	cmpb	$0x1, %al
  43fcc2: 89 4c 24 24                  	movl	%ecx, 0x24(%esp)
  43fcc6: 75 14                        	jne	0x43fcdc <.text+0x3ecdc>
  43fcc8: 8b 94 24 84 00 00 00         	movl	0x84(%esp), %edx
  43fccf: 8b 6a 1c                     	movl	0x1c(%edx), %ebp
  43fcd2: 03 2d 78 c8 56 00            	addl	0x56c878, %ebp
  43fcd8: 88 44 24 17                  	movb	%al, 0x17(%esp)
  43fcdc: 84 c0                        	testb	%al, %al
  43fcde: 75 79                        	jne	0x43fd59 <.text+0x3ed59>
  43fce0: 8b 94 24 90 00 00 00         	movl	0x90(%esp), %edx
  43fce7: 85 d2                        	testl	%edx, %edx
  43fce9: 74 22                        	je	0x43fd0d <.text+0x3ed0d>
  43fceb: 8b 3d 78 c3 56 00            	movl	0x56c378, %edi
  43fcf1: 33 c0                        	xorl	%eax, %eax
  43fcf3: 85 ff                        	testl	%edi, %edi
  43fcf5: 7e 16                        	jle	0x43fd0d <.text+0x3ed0d>
  43fcf7: 3b 0c 85 7c c3 56 00         	cmpl	0x56c37c(,%eax,4), %ecx
  43fcfe: 74 09                        	je	0x43fd09 <.text+0x3ed09>
  43fd00: 83 c0 01                     	addl	$0x1, %eax
  43fd03: 3b c7                        	cmpl	%edi, %eax
  43fd05: 7c f0                        	jl	0x43fcf7 <.text+0x3ecf7>
  43fd07: eb 04                        	jmp	0x43fd0d <.text+0x3ed0d>
  43fd09: c6 04 10 01                  	movb	$0x1, (%eax,%edx)
  43fd0d: 33 ff                        	xorl	%edi, %edi
  43fd0f: 39 3d 78 c3 56 00            	cmpl	%edi, 0x56c378
  43fd15: 7e 42                        	jle	0x43fd59 <.text+0x3ed59>
  43fd17: eb 07                        	jmp	0x43fd20 <.text+0x3ed20>
  43fd19: 8d a4 24 00 00 00 00         	leal	(%esp), %esp
  43fd20: 8b 44 24 24                  	movl	0x24(%esp), %eax
  43fd24: 8b 8c 24 8c 00 00 00         	movl	0x8c(%esp), %ecx
  43fd2b: 8b 49 14                     	movl	0x14(%ecx), %ecx
  43fd2e: 50                           	pushl	%eax
  43fd2f: e8 1c 2e fd ff               	calll	0x412b50 <.text+0x11b50>
  43fd34: 85 c0                        	testl	%eax, %eax
  43fd36: 74 16                        	je	0x43fd4e <.text+0x3ed4e>
  43fd38: 8b 54 24 24                  	movl	0x24(%esp), %edx
  43fd3c: 8b 84 24 8c 00 00 00         	movl	0x8c(%esp), %eax
  43fd43: 8b 48 14                     	movl	0x14(%eax), %ecx
  43fd46: 52                           	pushl	%edx
  43fd47: e8 04 2e fd ff               	calll	0x412b50 <.text+0x11b50>
  43fd4c: 8b d8                        	movl	%eax, %ebx
  43fd4e: 83 c7 01                     	addl	$0x1, %edi
  43fd51: 3b 3d 78 c3 56 00            	cmpl	0x56c378, %edi
  43fd57: 7c c7                        	jl	0x43fd20 <.text+0x3ed20>
  43fd59: 80 7c 24 17 01               	cmpb	$0x1, 0x17(%esp)
  43fd5e: 75 07                        	jne	0x43fd67 <.text+0x3ed67>
  43fd60: 8b 4e 08                     	movl	0x8(%esi), %ecx
  43fd63: 89 4c 24 28                  	movl	%ecx, 0x28(%esp)
  43fd67: c6 44 24 17 00               	movb	$0x0, 0x17(%esp)
  43fd6c: 8b 46 04                     	movl	0x4(%esi), %eax
  43fd6f: 83 e8 03                     	subl	$0x3, %eax
  43fd72: 0f 84 64 01 00 00            	je	0x43fedc <.text+0x3eedc>
  43fd78: 83 e8 01                     	subl	$0x1, %eax
  43fd7b: 0f 84 82 00 00 00            	je	0x43fe03 <.text+0x3ee03>
  43fd81: 83 e8 01                     	subl	$0x1, %eax
  43fd84: 0f 85 2f 02 00 00            	jne	0x43ffb9 <.text+0x3efb9>
  43fd8a: 8b 83 24 01 00 00            	movl	0x124(%ebx), %eax
  43fd90: 03 46 0c                     	addl	0xc(%esi), %eax
  43fd93: 8b 94 24 84 00 00 00         	movl	0x84(%esp), %edx
  43fd9a: 8b 7a 04                     	movl	0x4(%edx), %edi
  43fd9d: 8b 8c 24 88 00 00 00         	movl	0x88(%esp), %ecx
  43fda4: 03 01                        	addl	(%ecx), %eax
  43fda6: 8b 93 20 01 00 00            	movl	0x120(%ebx), %edx
  43fdac: 2b 54 24 28                  	subl	0x28(%esp), %edx
  43fdb0: 03 7c 24 20                  	addl	0x20(%esp), %edi
  43fdb4: 03 56 08                     	addl	0x8(%esi), %edx
  43fdb7: 03 c5                        	addl	%ebp, %eax
  43fdb9: 50                           	pushl	%eax
  43fdba: 8b 46 14                     	movl	0x14(%esi), %eax
  43fdbd: 52                           	pushl	%edx
  43fdbe: 50                           	pushl	%eax
  43fdbf: 8b cf                        	movl	%edi, %ecx
  43fdc1: e8 ea d6 fc ff               	calll	0x40d4b0 <.text+0xc4b0>
  43fdc6: 57                           	pushl	%edi
  43fdc7: 8b cb                        	movl	%ebx, %ecx
  43fdc9: e8 f2 2f fd ff               	calll	0x412dc0 <.text+0x11dc0>
  43fdce: 8a 8c 24 a8 00 00 00         	movb	0xa8(%esp), %cl
  43fdd5: 8b 57 10                     	movl	0x10(%edi), %edx
  43fdd8: 88 0f                        	movb	%cl, (%edi)
  43fdda: 8b 46 0c                     	movl	0xc(%esi), %eax
  43fddd: 8b 4e 08                     	movl	0x8(%esi), %ecx
  43fde0: 03 d0                        	addl	%eax, %edx
  43fde2: 03 d5                        	addl	%ebp, %edx
  43fde4: 52                           	pushl	%edx
  43fde5: 8b 57 0c                     	movl	0xc(%edi), %edx
  43fde8: 03 d1                        	addl	%ecx, %edx
  43fdea: 52                           	pushl	%edx
  43fdeb: 03 c5                        	addl	%ebp, %eax
  43fded: 50                           	pushl	%eax
  43fdee: 51                           	pushl	%ecx
  43fdef: 8d 44 24 44                  	leal	0x44(%esp), %eax
  43fdf3: 50                           	pushl	%eax
  43fdf4: ff 15 24 33 4a 00            	calll	*0x4a3324
  43fdfa: 8d 4c 24 34                  	leal	0x34(%esp), %ecx
  43fdfe: e9 83 01 00 00               	jmp	0x43ff86 <.text+0x3ef86>
  43fe03: 8b 84 24 84 00 00 00         	movl	0x84(%esp), %eax
  43fe0a: 8b 78 04                     	movl	0x4(%eax), %edi
  43fe0d: 03 7c 24 20                  	addl	0x20(%esp), %edi
  43fe11: 83 78 20 00                  	cmpl	$0x0, 0x20(%eax)
  43fe15: 8b 84 24 98 00 00 00         	movl	0x98(%esp), %eax
  43fe1c: 74 07                        	je	0x43fe25 <.text+0x3ee25>
  43fe1e: 8b 84 24 9c 00 00 00         	movl	0x9c(%esp), %eax
  43fe25: 83 f8 ff                     	cmpl	$-0x1, %eax
  43fe28: 75 03                        	jne	0x43fe2d <.text+0x3ee2d>
  43fe2a: 8b 46 18                     	movl	0x18(%esi), %eax
  43fe2d: 8b 0d 0c c9 56 00            	movl	0x56c90c, %ecx
  43fe33: 8b 93 24 01 00 00            	movl	0x124(%ebx), %edx
  43fe39: 03 56 0c                     	addl	0xc(%esi), %edx
  43fe3c: 6a ff                        	pushl	$-0x1
  43fe3e: 6a ff                        	pushl	$-0x1
  43fe40: 51                           	pushl	%ecx
  43fe41: 8b 8b 20 01 00 00            	movl	0x120(%ebx), %ecx
  43fe47: 2b 4c 24 34                  	subl	0x34(%esp), %ecx
  43fe4b: 50                           	pushl	%eax
  43fe4c: 8b 84 24 98 00 00 00         	movl	0x98(%esp), %eax
  43fe53: 03 10                        	addl	(%eax), %edx
  43fe55: 03 4e 08                     	addl	0x8(%esi), %ecx
  43fe58: 03 d5                        	addl	%ebp, %edx
  43fe5a: 52                           	pushl	%edx
  43fe5b: 8b 56 14                     	movl	0x14(%esi), %edx
  43fe5e: 51                           	pushl	%ecx
  43fe5f: 52                           	pushl	%edx
  43fe60: 8b cf                        	movl	%edi, %ecx
  43fe62: e8 89 d6 fc ff               	calll	0x40d4f0 <.text+0xc4f0>
  43fe67: 57                           	pushl	%edi
  43fe68: 8b cb                        	movl	%ebx, %ecx
  43fe6a: e8 51 2f fd ff               	calll	0x412dc0 <.text+0x11dc0>
  43fe6f: 8a 84 24 a8 00 00 00         	movb	0xa8(%esp), %al
  43fe76: 8b 57 10                     	movl	0x10(%edi), %edx
  43fe79: 88 07                        	movb	%al, (%edi)
  43fe7b: 8b 46 0c                     	movl	0xc(%esi), %eax
  43fe7e: 8b 4e 08                     	movl	0x8(%esi), %ecx
  43fe81: 03 d0                        	addl	%eax, %edx
  43fe83: 03 d5                        	addl	%ebp, %edx
  43fe85: 52                           	pushl	%edx
  43fe86: 8b 57 0c                     	movl	0xc(%edi), %edx
  43fe89: 03 d1                        	addl	%ecx, %edx
  43fe8b: 52                           	pushl	%edx
  43fe8c: 03 c5                        	addl	%ebp, %eax
  43fe8e: 50                           	pushl	%eax
  43fe8f: 51                           	pushl	%ecx
  43fe90: 8d 44 24 54                  	leal	0x54(%esp), %eax
  43fe94: 50                           	pushl	%eax
  43fe95: ff 15 24 33 4a 00            	calll	*0x4a3324
  43fe9b: 8b 84 24 84 00 00 00         	movl	0x84(%esp), %eax
  43fea2: 8d 4c 24 44                  	leal	0x44(%esp), %ecx
  43fea6: 51                           	pushl	%ecx
  43fea7: 83 c0 10                     	addl	$0x10, %eax
  43feaa: 50                           	pushl	%eax
  43feab: 50                           	pushl	%eax
  43feac: ff 15 d0 32 4a 00            	calll	*0x4a32d0
  43feb2: 8b 47 04                     	movl	0x4(%edi), %eax
  43feb5: 03 47 0c                     	addl	0xc(%edi), %eax
  43feb8: 85 ed                        	testl	%ebp, %ebp
  43feba: 89 44 24 18                  	movl	%eax, 0x18(%esp)
  43febe: 0f 85 f5 00 00 00            	jne	0x43ffb9 <.text+0x3efb9>
  43fec4: 39 6e 0c                     	cmpl	%ebp, 0xc(%esi)
  43fec7: 0f 85 ec 00 00 00            	jne	0x43ffb9 <.text+0x3efb9>
  43fecd: 39 44 24 1c                  	cmpl	%eax, 0x1c(%esp)
  43fed1: 0f 8d e2 00 00 00            	jge	0x43ffb9 <.text+0x3efb9>
  43fed7: e9 d9 00 00 00               	jmp	0x43ffb5 <.text+0x3efb5>
  43fedc: 80 7e 10 00                  	cmpb	$0x0, 0x10(%esi)
  43fee0: 8d 4e 10                     	leal	0x10(%esi), %ecx
  43fee3: 0f 84 d0 00 00 00            	je	0x43ffb9 <.text+0x3efb9>
  43fee9: 8b 84 24 84 00 00 00         	movl	0x84(%esp), %eax
  43fef0: 8b 78 04                     	movl	0x4(%eax), %edi
  43fef3: 03 7c 24 20                  	addl	0x20(%esp), %edi
  43fef7: 83 78 20 00                  	cmpl	$0x0, 0x20(%eax)
  43fefb: 8b 84 24 98 00 00 00         	movl	0x98(%esp), %eax
  43ff02: 74 07                        	je	0x43ff0b <.text+0x3ef0b>
  43ff04: 8b 84 24 9c 00 00 00         	movl	0x9c(%esp), %eax
  43ff0b: 83 f8 ff                     	cmpl	$-0x1, %eax
  43ff0e: 75 03                        	jne	0x43ff13 <.text+0x3ef13>
  43ff10: 8b 46 18                     	movl	0x18(%esi), %eax
  43ff13: 8b 15 0c c9 56 00            	movl	0x56c90c, %edx
  43ff19: 6a ff                        	pushl	$-0x1
  43ff1b: 6a ff                        	pushl	$-0x1
  43ff1d: 52                           	pushl	%edx
  43ff1e: 8b 93 24 01 00 00            	movl	0x124(%ebx), %edx
  43ff24: 03 56 0c                     	addl	0xc(%esi), %edx
  43ff27: 50                           	pushl	%eax
  43ff28: 8b 46 14                     	movl	0x14(%esi), %eax
  43ff2b: 50                           	pushl	%eax
  43ff2c: 8b 84 24 9c 00 00 00         	movl	0x9c(%esp), %eax
  43ff33: 03 10                        	addl	(%eax), %edx
  43ff35: 03 d5                        	addl	%ebp, %edx
  43ff37: 52                           	pushl	%edx
  43ff38: 8b 93 20 01 00 00            	movl	0x120(%ebx), %edx
  43ff3e: 2b 54 24 40                  	subl	0x40(%esp), %edx
  43ff42: 03 56 08                     	addl	0x8(%esi), %edx
  43ff45: 52                           	pushl	%edx
  43ff46: 51                           	pushl	%ecx
  43ff47: 8b cf                        	movl	%edi, %ecx
  43ff49: e8 f2 a1 fc ff               	calll	0x40a140 <.text+0x9140>
  43ff4e: 57                           	pushl	%edi
  43ff4f: 8b cb                        	movl	%ebx, %ecx
  43ff51: e8 6a 2e fd ff               	calll	0x412dc0 <.text+0x11dc0>
  43ff56: 8a 84 24 a8 00 00 00         	movb	0xa8(%esp), %al
  43ff5d: 8b 57 10                     	movl	0x10(%edi), %edx
  43ff60: 88 07                        	movb	%al, (%edi)
  43ff62: 8b 46 0c                     	movl	0xc(%esi), %eax
  43ff65: 8b 4e 08                     	movl	0x8(%esi), %ecx
  43ff68: 03 d0                        	addl	%eax, %edx
  43ff6a: 03 d5                        	addl	%ebp, %edx
  43ff6c: 52                           	pushl	%edx
  43ff6d: 8b 57 0c                     	movl	0xc(%edi), %edx
  43ff70: 03 d1                        	addl	%ecx, %edx
  43ff72: 52                           	pushl	%edx
  43ff73: 03 c5                        	addl	%ebp, %eax
  43ff75: 50                           	pushl	%eax
  43ff76: 51                           	pushl	%ecx
  43ff77: 8d 44 24 64                  	leal	0x64(%esp), %eax
  43ff7b: 50                           	pushl	%eax
  43ff7c: ff 15 24 33 4a 00            	calll	*0x4a3324
  43ff82: 8d 4c 24 54                  	leal	0x54(%esp), %ecx
  43ff86: 8b 84 24 84 00 00 00         	movl	0x84(%esp), %eax
  43ff8d: 51                           	pushl	%ecx
  43ff8e: 83 c0 10                     	addl	$0x10, %eax
  43ff91: 50                           	pushl	%eax
  43ff92: 50                           	pushl	%eax
  43ff93: ff 15 d0 32 4a 00            	calll	*0x4a32d0
  43ff99: 8b 47 04                     	movl	0x4(%edi), %eax
  43ff9c: 03 47 0c                     	addl	0xc(%edi), %eax
  43ff9f: 85 ed                        	testl	%ebp, %ebp
  43ffa1: 89 44 24 18                  	movl	%eax, 0x18(%esp)
  43ffa5: 75 12                        	jne	0x43ffb9 <.text+0x3efb9>
  43ffa7: 83 7e 0c 00                  	cmpl	$0x0, 0xc(%esi)
  43ffab: 75 0c                        	jne	0x43ffb9 <.text+0x3efb9>
  43ffad: 8b d0                        	movl	%eax, %edx
  43ffaf: 39 54 24 1c                  	cmpl	%edx, 0x1c(%esp)
  43ffb3: 7d 04                        	jge	0x43ffb9 <.text+0x3efb9>
  43ffb5: 89 44 24 1c                  	movl	%eax, 0x1c(%esp)
  43ffb9: 8b 44 24 30                  	movl	0x30(%esp), %eax
  43ffbd: 8b 8c 24 84 00 00 00         	movl	0x84(%esp), %ecx
  43ffc4: 83 44 24 2c 1c               	addl	$0x1c, 0x2c(%esp)
  43ffc9: 81 44 24 20 98 00 00 00      	addl	$0x98, 0x20(%esp)
  43ffd1: 83 c0 01                     	addl	$0x1, %eax
  43ffd4: 3b 41 0c                     	cmpl	0xc(%ecx), %eax
  43ffd7: 89 44 24 30                  	movl	%eax, 0x30(%esp)
  43ffdb: 0f 8c 8f fc ff ff            	jl	0x43fc70 <.text+0x3ec70>
  43ffe1: 8b 84 24 84 00 00 00         	movl	0x84(%esp), %eax
  43ffe8: 8b 15 74 c8 56 00            	movl	0x56c874, %edx
  43ffee: 01 50 1c                     	addl	%edx, 0x1c(%eax)
  43fff1: 8b f8                        	movl	%eax, %edi
  43fff3: e9 fd 01 00 00               	jmp	0x4401f5 <.text+0x3f1f5>
  43fff8: 8b 47 08                     	movl	0x8(%edi), %eax
  43fffb: 33 d2                        	xorl	%edx, %edx
  43fffd: 39 28                        	cmpl	%ebp, (%eax)
  43ffff: 55                           	pushl	%ebp
  440000: 55                           	pushl	%ebp
  440001: 0f 94 c2                     	sete	%dl
  440004: 55                           	pushl	%ebp
  440005: 8d 47 10                     	leal	0x10(%edi), %eax
  440008: 55                           	pushl	%ebp
  440009: 50                           	pushl	%eax
  44000a: 89 6c 24 3c                  	movl	%ebp, 0x3c(%esp)
  44000e: 89 94 24 98 00 00 00         	movl	%edx, 0x98(%esp)
  440015: ff d1                        	calll	*%ecx
  440017: 39 6f 0c                     	cmpl	%ebp, 0xc(%edi)
  44001a: 8b 9c 24 94 00 00 00         	movl	0x94(%esp), %ebx
  440021: 89 6c 24 20                  	movl	%ebp, 0x20(%esp)
  440025: 0f 8e b8 01 00 00            	jle	0x4401e3 <.text+0x3f1e3>
  44002b: 8b 15 78 c3 56 00            	movl	0x56c378, %edx
  440031: 89 6c 24 24                  	movl	%ebp, 0x24(%esp)
  440035: 89 6c 24 2c                  	movl	%ebp, 0x2c(%esp)
  440039: 8d a4 24 00 00 00 00         	leal	(%esp), %esp
  440040: 8b 6f 08                     	movl	0x8(%edi), %ebp
  440043: 03 6c 24 2c                  	addl	0x2c(%esp), %ebp
  440047: 8b 77 04                     	movl	0x4(%edi), %esi
  44004a: 8b 45 04                     	movl	0x4(%ebp), %eax
  44004d: 03 74 24 24                  	addl	0x24(%esp), %esi
  440051: 83 f8 03                     	cmpl	$0x3, %eax
  440054: 0f 8c 68 01 00 00            	jl	0x4401c2 <.text+0x3f1c2>
  44005a: 83 f8 05                     	cmpl	$0x5, %eax
  44005d: 0f 8f 5f 01 00 00            	jg	0x4401c2 <.text+0x3f1c2>
  440063: 8b 45 00                     	movl	(%ebp), %eax
  440066: 39 84 24 84 00 00 00         	cmpl	%eax, 0x84(%esp)
  44006d: 0f 84 b6 00 00 00            	je	0x440129 <.text+0x3f129>
  440073: 89 84 24 84 00 00 00         	movl	%eax, 0x84(%esp)
  44007a: 8b 84 24 8c 00 00 00         	movl	0x8c(%esp), %eax
  440081: 8a 40 68                     	movb	0x68(%eax), %al
  440084: 3c 01                        	cmpb	$0x1, %al
  440086: 75 0d                        	jne	0x440095 <.text+0x3f095>
  440088: 8b 4f 1c                     	movl	0x1c(%edi), %ecx
  44008b: 03 0d 78 c8 56 00            	addl	0x56c878, %ecx
  440091: 89 4c 24 28                  	movl	%ecx, 0x28(%esp)
  440095: 84 c0                        	testb	%al, %al
  440097: 0f 85 8c 00 00 00            	jne	0x440129 <.text+0x3f129>
  44009d: 33 c0                        	xorl	%eax, %eax
  44009f: 85 d2                        	testl	%edx, %edx
  4400a1: 7e 2a                        	jle	0x4400cd <.text+0x3f0cd>
  4400a3: 8b 8c 24 84 00 00 00         	movl	0x84(%esp), %ecx
  4400aa: 3b 0c 85 7c c3 56 00         	cmpl	0x56c37c(,%eax,4), %ecx
  4400b1: 74 09                        	je	0x4400bc <.text+0x3f0bc>
  4400b3: 83 c0 01                     	addl	$0x1, %eax
  4400b6: 3b c2                        	cmpl	%edx, %eax
  4400b8: 7c f0                        	jl	0x4400aa <.text+0x3f0aa>
  4400ba: eb 11                        	jmp	0x4400cd <.text+0x3f0cd>
  4400bc: 8b 8c 24 90 00 00 00         	movl	0x90(%esp), %ecx
  4400c3: c6 04 08 01                  	movb	$0x1, (%eax,%ecx)
  4400c7: 8b 15 78 c3 56 00            	movl	0x56c378, %edx
  4400cd: 85 d2                        	testl	%edx, %edx
  4400cf: c7 44 24 30 00 00 00 00      	movl	$0x0, 0x30(%esp)
  4400d7: 7e 50                        	jle	0x440129 <.text+0x3f129>
  4400d9: 8d a4 24 00 00 00 00         	leal	(%esp), %esp
  4400e0: 8b 94 24 84 00 00 00         	movl	0x84(%esp), %edx
  4400e7: 8b 84 24 8c 00 00 00         	movl	0x8c(%esp), %eax
  4400ee: 8b 48 14                     	movl	0x14(%eax), %ecx
  4400f1: 52                           	pushl	%edx
  4400f2: e8 59 2a fd ff               	calll	0x412b50 <.text+0x11b50>
  4400f7: 85 c0                        	testl	%eax, %eax
  4400f9: 74 19                        	je	0x440114 <.text+0x3f114>
  4400fb: 8b 8c 24 84 00 00 00         	movl	0x84(%esp), %ecx
  440102: 8b 94 24 8c 00 00 00         	movl	0x8c(%esp), %edx
  440109: 51                           	pushl	%ecx
  44010a: 8b 4a 14                     	movl	0x14(%edx), %ecx
  44010d: e8 3e 2a fd ff               	calll	0x412b50 <.text+0x11b50>
  440112: 8b d8                        	movl	%eax, %ebx
  440114: 8b 44 24 30                  	movl	0x30(%esp), %eax
  440118: 8b 15 78 c3 56 00            	movl	0x56c378, %edx
  44011e: 83 c0 01                     	addl	$0x1, %eax
  440121: 3b c2                        	cmpl	%edx, %eax
  440123: 89 44 24 30                  	movl	%eax, 0x30(%esp)
  440127: 7c b7                        	jl	0x4400e0 <.text+0x3f0e0>
  440129: 85 f6                        	testl	%esi, %esi
  44012b: 0f 84 91 00 00 00            	je	0x4401c2 <.text+0x3f1c2>
  440131: 8b 45 04                     	movl	0x4(%ebp), %eax
  440134: 83 c0 fd                     	addl	$-0x3, %eax
  440137: 83 f8 02                     	cmpl	$0x2, %eax
  44013a: 0f 87 82 00 00 00            	ja	0x4401c2 <.text+0x3f1c2>
  440140: 8b 83 24 01 00 00            	movl	0x124(%ebx), %eax
  440146: 8b 8c 24 88 00 00 00         	movl	0x88(%esp), %ecx
  44014d: 03 01                        	addl	(%ecx), %eax
  44014f: 8b 4c 24 28                  	movl	0x28(%esp), %ecx
  440153: 8a 94 24 a8 00 00 00         	movb	0xa8(%esp), %dl
  44015a: 03 c1                        	addl	%ecx, %eax
  44015c: 03 45 0c                     	addl	0xc(%ebp), %eax
  44015f: 88 16                        	movb	%dl, (%esi)
  440161: 8b 56 10                     	movl	0x10(%esi), %edx
  440164: 89 46 08                     	movl	%eax, 0x8(%esi)
  440167: 8b 45 0c                     	movl	0xc(%ebp), %eax
  44016a: 03 d0                        	addl	%eax, %edx
  44016c: 03 d1                        	addl	%ecx, %edx
  44016e: 8b 4d 08                     	movl	0x8(%ebp), %ecx
  440171: 52                           	pushl	%edx
  440172: 8b 56 0c                     	movl	0xc(%esi), %edx
  440175: 03 d1                        	addl	%ecx, %edx
  440177: 52                           	pushl	%edx
  440178: 8b 54 24 30                  	movl	0x30(%esp), %edx
  44017c: 03 c2                        	addl	%edx, %eax
  44017e: 50                           	pushl	%eax
  44017f: 51                           	pushl	%ecx
  440180: 8d 44 24 64                  	leal	0x64(%esp), %eax
  440184: 50                           	pushl	%eax
  440185: ff 15 24 33 4a 00            	calll	*0x4a3324
  44018b: 8d 4c 24 54                  	leal	0x54(%esp), %ecx
  44018f: 51                           	pushl	%ecx
  440190: 8d 47 10                     	leal	0x10(%edi), %eax
  440193: 50                           	pushl	%eax
  440194: 50                           	pushl	%eax
  440195: ff 15 d0 32 4a 00            	calll	*0x4a32d0
  44019b: 8b 46 04                     	movl	0x4(%esi), %eax
  44019e: 03 46 0c                     	addl	0xc(%esi), %eax
  4401a1: 83 7c 24 28 00               	cmpl	$0x0, 0x28(%esp)
  4401a6: 89 44 24 18                  	movl	%eax, 0x18(%esp)
  4401aa: 75 10                        	jne	0x4401bc <.text+0x3f1bc>
  4401ac: 83 7d 0c 00                  	cmpl	$0x0, 0xc(%ebp)
  4401b0: 75 0a                        	jne	0x4401bc <.text+0x3f1bc>
  4401b2: 39 44 24 1c                  	cmpl	%eax, 0x1c(%esp)
  4401b6: 7d 04                        	jge	0x4401bc <.text+0x3f1bc>
  4401b8: 89 44 24 1c                  	movl	%eax, 0x1c(%esp)
  4401bc: 8b 15 78 c3 56 00            	movl	0x56c378, %edx
  4401c2: 8b 44 24 20                  	movl	0x20(%esp), %eax
  4401c6: 83 44 24 2c 1c               	addl	$0x1c, 0x2c(%esp)
  4401cb: 81 44 24 24 98 00 00 00      	addl	$0x98, 0x24(%esp)
  4401d3: 83 c0 01                     	addl	$0x1, %eax
  4401d6: 3b 47 0c                     	cmpl	0xc(%edi), %eax
  4401d9: 89 44 24 20                  	movl	%eax, 0x20(%esp)
  4401dd: 0f 8c 5d fe ff ff            	jl	0x440040 <.text+0x3f040>
  4401e3: 8b 15 74 c8 56 00            	movl	0x56c874, %edx
  4401e9: 01 57 1c                     	addl	%edx, 0x1c(%edi)
  4401ec: eb 07                        	jmp	0x4401f5 <.text+0x3f1f5>
  4401ee: 8b 9c 24 94 00 00 00         	movl	0x94(%esp), %ebx
  4401f5: 83 7f 20 00                  	cmpl	$0x0, 0x20(%edi)
  4401f9: 0f 84 27 02 00 00            	je	0x440426 <.text+0x3f426>
  4401ff: 8b 47 24                     	movl	0x24(%edi), %eax
  440202: 85 c0                        	testl	%eax, %eax
  440204: 89 84 24 84 00 00 00         	movl	%eax, 0x84(%esp)
  44020b: 0f 84 15 02 00 00            	je	0x440426 <.text+0x3f426>
  440211: 8b 84 24 8c 00 00 00         	movl	0x8c(%esp), %eax
  440218: 33 ed                        	xorl	%ebp, %ebp
  44021a: 80 b8 f8 00 00 00 01         	cmpb	$0x1, 0xf8(%eax)
  440221: 0f 85 45 01 00 00            	jne	0x44036c <.text+0x3f36c>
  440227: 8b 88 f4 00 00 00            	movl	0xf4(%eax), %ecx
  44022d: 51                           	pushl	%ecx
  44022e: 8b 48 14                     	movl	0x14(%eax), %ecx
  440231: e8 1a 29 fd ff               	calll	0x412b50 <.text+0x11b50>
  440236: 85 c0                        	testl	%eax, %eax
  440238: 0f 84 2e 01 00 00            	je	0x44036c <.text+0x3f36c>
  44023e: 8b 94 24 84 00 00 00         	movl	0x84(%esp), %edx
  440245: 8b b2 d4 00 00 00            	movl	0xd4(%edx), %esi
  44024b: 85 f6                        	testl	%esi, %esi
  44024d: 0f 84 82 00 00 00            	je	0x4402d5 <.text+0x3f2d5>
  440253: a1 7c c8 56 00               	movl	0x56c87c, %eax
  440258: 8b 4c 24 18                  	movl	0x18(%esp), %ecx
  44025c: 8b 54 24 1c                  	movl	0x1c(%esp), %edx
  440260: 50                           	pushl	%eax
  440261: 51                           	pushl	%ecx
  440262: 8b 4f 10                     	movl	0x10(%edi), %ecx
  440265: 52                           	pushl	%edx
  440266: 8b 57 14                     	movl	0x14(%edi), %edx
  440269: 8d 6f 10                     	leal	0x10(%edi), %ebp
  44026c: 83 ec 10                     	subl	$0x10, %esp
  44026f: 8b c4                        	movl	%esp, %eax
  440271: 89 08                        	movl	%ecx, (%eax)
  440273: 8b 4d 08                     	movl	0x8(%ebp), %ecx
  440276: 89 50 04                     	movl	%edx, 0x4(%eax)
  440279: 8b 55 0c                     	movl	0xc(%ebp), %edx
  44027c: 89 48 08                     	movl	%ecx, 0x8(%eax)
  44027f: 56                           	pushl	%esi
  440280: 89 50 0c                     	movl	%edx, 0xc(%eax)
  440283: e8 c8 3b ff ff               	calll	0x433e50 <.text+0x32e50>
  440288: 8b 46 08                     	movl	0x8(%esi), %eax
  44028b: 8b 56 10                     	movl	0x10(%esi), %edx
  44028e: 8b 4e 04                     	movl	0x4(%esi), %ecx
  440291: 83 c4 20                     	addl	$0x20, %esp
  440294: 03 d0                        	addl	%eax, %edx
  440296: 52                           	pushl	%edx
  440297: 8b 56 0c                     	movl	0xc(%esi), %edx
  44029a: 03 d1                        	addl	%ecx, %edx
  44029c: 52                           	pushl	%edx
  44029d: 50                           	pushl	%eax
  44029e: 51                           	pushl	%ecx
  44029f: 8d 44 24 64                  	leal	0x64(%esp), %eax
  4402a3: 50                           	pushl	%eax
  4402a4: ff 15 24 33 4a 00            	calll	*0x4a3324
  4402aa: 8d 4c 24 54                  	leal	0x54(%esp), %ecx
  4402ae: 51                           	pushl	%ecx
  4402af: 55                           	pushl	%ebp
  4402b0: 55                           	pushl	%ebp
  4402b1: ff 15 d0 32 4a 00            	calll	*0x4a32d0
  4402b7: 8b 93 20 01 00 00            	movl	0x120(%ebx), %edx
  4402bd: 01 56 04                     	addl	%edx, 0x4(%esi)
  4402c0: 8b 83 24 01 00 00            	movl	0x124(%ebx), %eax
  4402c6: 8b 8c 24 88 00 00 00         	movl	0x88(%esp), %ecx
  4402cd: 03 01                        	addl	(%ecx), %eax
  4402cf: 8b 6e 0c                     	movl	0xc(%esi), %ebp
  4402d2: 01 46 08                     	addl	%eax, 0x8(%esi)
  4402d5: 8b 94 24 84 00 00 00         	movl	0x84(%esp), %edx
  4402dc: 8b b2 d8 00 00 00            	movl	0xd8(%edx), %esi
  4402e2: 85 f6                        	testl	%esi, %esi
  4402e4: 0f 84 82 00 00 00            	je	0x44036c <.text+0x3f36c>
  4402ea: a1 7c c8 56 00               	movl	0x56c87c, %eax
  4402ef: 8b 4c 24 18                  	movl	0x18(%esp), %ecx
  4402f3: 8b 54 24 1c                  	movl	0x1c(%esp), %edx
  4402f7: 50                           	pushl	%eax
  4402f8: 51                           	pushl	%ecx
  4402f9: 8b 4f 10                     	movl	0x10(%edi), %ecx
  4402fc: 52                           	pushl	%edx
  4402fd: 8b 57 14                     	movl	0x14(%edi), %edx
  440300: 8d 6f 10                     	leal	0x10(%edi), %ebp
  440303: 83 ec 10                     	subl	$0x10, %esp
  440306: 8b c4                        	movl	%esp, %eax
  440308: 89 08                        	movl	%ecx, (%eax)
  44030a: 8b 4d 08                     	movl	0x8(%ebp), %ecx
  44030d: 89 50 04                     	movl	%edx, 0x4(%eax)
  440310: 8b 55 0c                     	movl	0xc(%ebp), %edx
  440313: 89 48 08                     	movl	%ecx, 0x8(%eax)
  440316: 56                           	pushl	%esi
  440317: 89 50 0c                     	movl	%edx, 0xc(%eax)
  44031a: e8 31 3b ff ff               	calll	0x433e50 <.text+0x32e50>
  44031f: 8b 46 08                     	movl	0x8(%esi), %eax
  440322: 8b 56 10                     	movl	0x10(%esi), %edx
  440325: 8b 4e 04                     	movl	0x4(%esi), %ecx
  440328: 83 c4 20                     	addl	$0x20, %esp
  44032b: 03 d0                        	addl	%eax, %edx
  44032d: 52                           	pushl	%edx
  44032e: 8b 56 0c                     	movl	0xc(%esi), %edx
  440331: 03 d1                        	addl	%ecx, %edx
  440333: 52                           	pushl	%edx
  440334: 50                           	pushl	%eax
  440335: 51                           	pushl	%ecx
  440336: 8d 44 24 74                  	leal	0x74(%esp), %eax
  44033a: 50                           	pushl	%eax
  44033b: ff 15 24 33 4a 00            	calll	*0x4a3324
  440341: 8d 4c 24 64                  	leal	0x64(%esp), %ecx
  440345: 51                           	pushl	%ecx
  440346: 55                           	pushl	%ebp
  440347: 55                           	pushl	%ebp
  440348: ff 15 d0 32 4a 00            	calll	*0x4a32d0
  44034e: 8b 93 20 01 00 00            	movl	0x120(%ebx), %edx
  440354: 01 56 04                     	addl	%edx, 0x4(%esi)
  440357: 8b 83 24 01 00 00            	movl	0x124(%ebx), %eax
  44035d: 8b 8c 24 88 00 00 00         	movl	0x88(%esp), %ecx
  440364: 03 01                        	addl	(%ecx), %eax
  440366: 8b 6e 0c                     	movl	0xc(%esi), %ebp
  440369: 01 46 08                     	addl	%eax, 0x8(%esi)
  44036c: 8b 94 24 8c 00 00 00         	movl	0x8c(%esp), %edx
  440373: dd 83 d8 00 00 00            	fldl	0xd8(%ebx)
  440379: 80 7a 68 01                  	cmpb	$0x1, 0x68(%edx)
  44037d: 75 5c                        	jne	0x4403db <.text+0x3f3db>
  44037f: e8 1c 49 02 00               	calll	0x464ca0 <.text+0x63ca0>
  440384: dd 83 d0 00 00 00            	fldl	0xd0(%ebx)
  44038a: 8b 8b 24 01 00 00            	movl	0x124(%ebx), %ecx
  440390: 8b 94 24 88 00 00 00         	movl	0x88(%esp), %edx
  440397: 8b 2a                        	movl	(%edx), %ebp
  440399: 8d 34 08                     	leal	(%eax,%ecx), %esi
  44039c: e8 ff 48 02 00               	calll	0x464ca0 <.text+0x63ca0>
  4403a1: 8b 57 1c                     	movl	0x1c(%edi), %edx
  4403a4: 2b 15 74 c8 56 00            	subl	0x56c874, %edx
  4403aa: 8b 8b 20 01 00 00            	movl	0x120(%ebx), %ecx
  4403b0: 03 d6                        	addl	%esi, %edx
  4403b2: 03 d5                        	addl	%ebp, %edx
  4403b4: 52                           	pushl	%edx
  4403b5: 8b 57 14                     	movl	0x14(%edi), %edx
  4403b8: 03 c1                        	addl	%ecx, %eax
  4403ba: 8b 4f 18                     	movl	0x18(%edi), %ecx
  4403bd: 03 c8                        	addl	%eax, %ecx
  4403bf: 51                           	pushl	%ecx
  4403c0: 8b 4f 10                     	movl	0x10(%edi), %ecx
  4403c3: 03 d6                        	addl	%esi, %edx
  4403c5: 03 d5                        	addl	%ebp, %edx
  4403c7: 52                           	pushl	%edx
  4403c8: 8b 94 24 90 00 00 00         	movl	0x90(%esp), %edx
  4403cf: 03 c8                        	addl	%eax, %ecx
  4403d1: 51                           	pushl	%ecx
  4403d2: 81 c2 a8 00 00 00            	addl	$0xa8, %edx
  4403d8: 52                           	pushl	%edx
  4403d9: eb 45                        	jmp	0x440420 <.text+0x3f420>
  4403db: e8 c0 48 02 00               	calll	0x464ca0 <.text+0x63ca0>
  4403e0: dd 83 d0 00 00 00            	fldl	0xd0(%ebx)
  4403e6: 8b f0                        	movl	%eax, %esi
  4403e8: e8 b3 48 02 00               	calll	0x464ca0 <.text+0x63ca0>
  4403ed: 8b 8b 2c 01 00 00            	movl	0x12c(%ebx), %ecx
  4403f3: 8b 93 28 01 00 00            	movl	0x128(%ebx), %edx
  4403f9: 03 ce                        	addl	%esi, %ecx
  4403fb: 51                           	pushl	%ecx
  4403fc: 8b 8b 24 01 00 00            	movl	0x124(%ebx), %ecx
  440402: 03 d0                        	addl	%eax, %edx
  440404: 52                           	pushl	%edx
  440405: 8b 93 20 01 00 00            	movl	0x120(%ebx), %edx
  44040b: 2b d5                        	subl	%ebp, %edx
  44040d: 03 ce                        	addl	%esi, %ecx
  44040f: 03 d0                        	addl	%eax, %edx
  440411: 8b 84 24 8c 00 00 00         	movl	0x8c(%esp), %eax
  440418: 51                           	pushl	%ecx
  440419: 52                           	pushl	%edx
  44041a: 05 a8 00 00 00               	addl	$0xa8, %eax
  44041f: 50                           	pushl	%eax
  440420: ff 15 24 33 4a 00            	calll	*0x4a3324
  440426: 8b 4c 24 74                  	movl	0x74(%esp), %ecx
  44042a: 64 89 0d 00 00 00 00         	movl	%ecx, %fs:0x0
  440431: 59                           	popl	%ecx
  440432: 5f                           	popl	%edi
  440433: 5e                           	popl	%esi
  440434: 5d                           	popl	%ebp
  440435: 5b                           	popl	%ebx
  440436: 83 c4 6c                     	addl	$0x6c, %esp
  440439: c3                           	retl
  44043a: cc                           	int3
  44043b: cc                           	int3
  44043c: cc                           	int3
  44043d: cc                           	int3
  44043e: cc                           	int3
  44043f: cc                           	int3


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  440440: 6a ff                        	pushl	$-0x1
  440442: 68 1b 1f 4a 00               	pushl	$0x4a1f1b               # imm = 0x4A1F1B
  440447: 64 a1 00 00 00 00            	movl	%fs:0x0, %eax
  44044d: 50                           	pushl	%eax
  44044e: b8 bc a4 00 00               	movl	$0xa4bc, %eax           # imm = 0xA4BC
  440453: e8 b8 51 02 00               	calll	0x465610 <.text+0x64610>
  440458: a1 48 ea 4b 00               	movl	0x4bea48, %eax
  44045d: 33 c4                        	xorl	%esp, %eax
  44045f: 89 84 24 b8 a4 00 00         	movl	%eax, 0xa4b8(%esp)
  440466: 53                           	pushl	%ebx
  440467: 55                           	pushl	%ebp
  440468: 56                           	pushl	%esi
  440469: 57                           	pushl	%edi
  44046a: a1 48 ea 4b 00               	movl	0x4bea48, %eax
  44046f: 33 c4                        	xorl	%esp, %eax
  440471: 50                           	pushl	%eax
  440472: 8d 84 24 d0 a4 00 00         	leal	0xa4d0(%esp), %eax
  440479: 64 a3 00 00 00 00            	movl	%eax, %fs:0x0
  44047f: 8b ac 24 e0 a4 00 00         	movl	0xa4e0(%esp), %ebp
  440486: 80 7d 68 00                  	cmpb	$0x0, 0x68(%ebp)
  44048a: 75 12                        	jne	0x44049e <.text+0x3f49e>
  44048c: 8b 85 40 01 08 00            	movl	0x80140(%ebp), %eax
  440492: 3b 85 3c 01 08 00            	cmpl	0x8013c(%ebp), %eax
  440498: 0f 84 77 13 00 00            	je	0x441815 <.text+0x40815>
  44049e: 8a 0d d0 87 55 00            	movb	0x5587d0, %cl
  4404a4: a1 ac 70 55 00               	movl	0x5570ac, %eax
  4404a9: 33 ff                        	xorl	%edi, %edi
  4404ab: 3b c7                        	cmpl	%edi, %eax
  4404ad: 88 4c 24 3f                  	movb	%cl, 0x3f(%esp)
  4404b1: c6 05 d0 87 55 00 00         	movb	$0x0, 0x5587d0
  4404b8: 89 7c 24 68                  	movl	%edi, 0x68(%esp)
  4404bc: 74 13                        	je	0x4404d1 <.text+0x3f4d1>
  4404be: 80 3d 89 c8 56 00 01         	cmpb	$0x1, 0x56c889
  4404c5: 75 0a                        	jne	0x4404d1 <.text+0x3f4d1>
  4404c7: 89 44 24 68                  	movl	%eax, 0x68(%esp)
  4404cb: 89 3d ac 70 55 00            	movl	%edi, 0x5570ac
  4404d1: 68 a0 70 55 00               	pushl	$0x5570a0               # imm = 0x5570A0
  4404d6: 8d 8c 24 fc 00 00 00         	leal	0xfc(%esp), %ecx
  4404dd: e8 0e a8 fd ff               	calll	0x41acf0 <.text+0x19cf0>
  4404e2: 8b 55 0c                     	movl	0xc(%ebp), %edx
  4404e5: 8b 4d 14                     	movl	0x14(%ebp), %ecx
  4404e8: 83 c8 ff                     	orl	$-0x1, %eax
  4404eb: 52                           	pushl	%edx
  4404ec: 89 bc 24 dc a4 00 00         	movl	%edi, 0xa4dc(%esp)
  4404f3: 89 7c 24 44                  	movl	%edi, 0x44(%esp)
  4404f7: 89 44 24 54                  	movl	%eax, 0x54(%esp)
  4404fb: 89 44 24 4c                  	movl	%eax, 0x4c(%esp)
  4404ff: e8 4c 26 fd ff               	calll	0x412b50 <.text+0x11b50>
  440504: 80 7d 5c 00                  	cmpb	$0x0, 0x5c(%ebp)
  440508: 8b d8                        	movl	%eax, %ebx
  44050a: 89 5c 24 34                  	movl	%ebx, 0x34(%esp)
  44050e: 74 0e                        	je	0x44051e <.text+0x3f51e>
  440510: 8b 45 60                     	movl	0x60(%ebp), %eax
  440513: 8b 4d 64                     	movl	0x64(%ebp), %ecx
  440516: 89 44 24 50                  	movl	%eax, 0x50(%esp)
  44051a: 89 4c 24 48                  	movl	%ecx, 0x48(%esp)
  44051e: 33 c0                        	xorl	%eax, %eax
  440520: 89 84 24 ac a4 00 00         	movl	%eax, 0xa4ac(%esp)
  440527: 89 84 24 b0 a4 00 00         	movl	%eax, 0xa4b0(%esp)
  44052e: 89 84 24 b4 a4 00 00         	movl	%eax, 0xa4b4(%esp)
  440535: 89 84 24 b8 a4 00 00         	movl	%eax, 0xa4b8(%esp)
  44053c: 89 84 24 9c a4 00 00         	movl	%eax, 0xa49c(%esp)
  440543: 89 84 24 a0 a4 00 00         	movl	%eax, 0xa4a0(%esp)
  44054a: 89 84 24 a4 a4 00 00         	movl	%eax, 0xa4a4(%esp)
  440551: 89 84 24 a8 a4 00 00         	movl	%eax, 0xa4a8(%esp)
  440558: 89 44 24 78                  	movl	%eax, 0x78(%esp)
  44055c: 89 44 24 7c                  	movl	%eax, 0x7c(%esp)
  440560: 89 84 24 80 00 00 00         	movl	%eax, 0x80(%esp)
  440567: 89 84 24 84 00 00 00         	movl	%eax, 0x84(%esp)
  44056e: 89 84 24 88 00 00 00         	movl	%eax, 0x88(%esp)
  440575: 89 84 24 8c 00 00 00         	movl	%eax, 0x8c(%esp)
  44057c: 89 84 24 90 00 00 00         	movl	%eax, 0x90(%esp)
  440583: 89 84 24 94 00 00 00         	movl	%eax, 0x94(%esp)
  44058a: 89 84 24 98 00 00 00         	movl	%eax, 0x98(%esp)
  440591: 89 84 24 9c 00 00 00         	movl	%eax, 0x9c(%esp)
  440598: 89 84 24 a0 00 00 00         	movl	%eax, 0xa0(%esp)
  44059f: 89 84 24 a4 00 00 00         	movl	%eax, 0xa4(%esp)
  4405a6: 89 84 24 a8 00 00 00         	movl	%eax, 0xa8(%esp)
  4405ad: 89 84 24 ac 00 00 00         	movl	%eax, 0xac(%esp)
  4405b4: 89 84 24 b0 00 00 00         	movl	%eax, 0xb0(%esp)
  4405bb: 89 84 24 b4 00 00 00         	movl	%eax, 0xb4(%esp)
  4405c2: 89 84 24 b8 00 00 00         	movl	%eax, 0xb8(%esp)
  4405c9: 89 84 24 bc 00 00 00         	movl	%eax, 0xbc(%esp)
  4405d0: 89 84 24 c0 00 00 00         	movl	%eax, 0xc0(%esp)
  4405d7: 89 84 24 c4 00 00 00         	movl	%eax, 0xc4(%esp)
  4405de: 89 84 24 c8 00 00 00         	movl	%eax, 0xc8(%esp)
  4405e5: 89 84 24 cc 00 00 00         	movl	%eax, 0xcc(%esp)
  4405ec: 89 84 24 d0 00 00 00         	movl	%eax, 0xd0(%esp)
  4405f3: 89 84 24 d4 00 00 00         	movl	%eax, 0xd4(%esp)
  4405fa: 89 84 24 d8 00 00 00         	movl	%eax, 0xd8(%esp)
  440601: 89 84 24 dc 00 00 00         	movl	%eax, 0xdc(%esp)
  440608: 89 84 24 e0 00 00 00         	movl	%eax, 0xe0(%esp)
  44060f: 89 84 24 e4 00 00 00         	movl	%eax, 0xe4(%esp)
  440616: 89 84 24 e8 00 00 00         	movl	%eax, 0xe8(%esp)
  44061d: 89 84 24 ec 00 00 00         	movl	%eax, 0xec(%esp)
  440624: 89 84 24 f0 00 00 00         	movl	%eax, 0xf0(%esp)
  44062b: 89 84 24 f4 00 00 00         	movl	%eax, 0xf4(%esp)
  440632: 89 bc 04 5c 03 00 00         	movl	%edi, 0x35c(%esp,%eax)
  440639: 89 bc 04 60 03 00 00         	movl	%edi, 0x360(%esp,%eax)
  440640: 89 bc 04 dc 03 00 00         	movl	%edi, 0x3dc(%esp,%eax)
  440647: 89 bc 04 e0 03 00 00         	movl	%edi, 0x3e0(%esp,%eax)
  44064e: 83 c0 08                     	addl	$0x8, %eax
  440651: 3d 80 00 00 00               	cmpl	$0x80, %eax
  440656: 72 da                        	jb	0x440632 <.text+0x3f632>
  440658: 80 7d 68 00                  	cmpb	$0x0, 0x68(%ebp)
  44065c: 74 12                        	je	0x440670 <.text+0x3f670>
  44065e: 8b 55 6c                     	movl	0x6c(%ebp), %edx
  440661: 8b 4d 14                     	movl	0x14(%ebp), %ecx
  440664: 52                           	pushl	%edx
  440665: e8 e6 24 fd ff               	calll	0x412b50 <.text+0x11b50>
  44066a: 8b d8                        	movl	%eax, %ebx
  44066c: 89 44 24 34                  	movl	%eax, 0x34(%esp)
  440670: 3b df                        	cmpl	%edi, %ebx
  440672: 0f 84 eb 01 00 00            	je	0x440863 <.text+0x3f863>
  440678: 39 3d 78 c3 56 00            	cmpl	%edi, 0x56c378
  44067e: 89 7c 24 28                  	movl	%edi, 0x28(%esp)
  440682: 0f 8e d2 01 00 00            	jle	0x44085a <.text+0x3f85a>
  440688: eb 06                        	jmp	0x440690 <.text+0x3f690>
  44068a: 8d 9b 00 00 00 00            	leal	(%ebx), %ebx
  440690: 8b 34 bd 7c c3 56 00         	movl	0x56c37c(,%edi,4), %esi
  440697: 8b 4d 14                     	movl	0x14(%ebp), %ecx
  44069a: 56                           	pushl	%esi
  44069b: e8 b0 24 fd ff               	calll	0x412b50 <.text+0x11b50>
  4406a0: 85 c0                        	testl	%eax, %eax
  4406a2: 0f 84 9f 01 00 00            	je	0x440847 <.text+0x3f847>
  4406a8: 8b 4d 14                     	movl	0x14(%ebp), %ecx
  4406ab: 56                           	pushl	%esi
  4406ac: e8 9f 24 fd ff               	calll	0x412b50 <.text+0x11b50>
  4406b1: 80 b8 f8 00 00 00 01         	cmpb	$0x1, 0xf8(%eax)
  4406b8: 75 08                        	jne	0x4406c2 <.text+0x3f6c2>
  4406ba: c6 84 3c ac a4 00 00 01      	movb	$0x1, 0xa4ac(%esp,%edi)
  4406c2: 68 a0 d9 4c 00               	pushl	$0x4cd9a0               # imm = 0x4CD9A0
  4406c7: ff 15 10 32 4a 00            	calll	*0x4a3210
  4406cd: 8b 4d 14                     	movl	0x14(%ebp), %ecx
  4406d0: 56                           	pushl	%esi
  4406d1: e8 7a 24 fd ff               	calll	0x412b50 <.text+0x11b50>
  4406d6: 8b 80 e0 10 00 00            	movl	0x10e0(%eax), %eax
  4406dc: 8b 5c 24 28                  	movl	0x28(%esp), %ebx
  4406e0: 89 44 9c 78                  	movl	%eax, 0x78(%esp,%ebx,4)
  4406e4: 33 c9                        	xorl	%ecx, %ecx
  4406e6: 83 c0 01                     	addl	$0x1, %eax
  4406e9: ba 04 00 00 00               	movl	$0x4, %edx
  4406ee: f7 e2                        	mull	%edx
  4406f0: 0f 90 c1                     	seto	%cl
  4406f3: f7 d9                        	negl	%ecx
  4406f5: 0b c8                        	orl	%eax, %ecx
  4406f7: 51                           	pushl	%ecx
  4406f8: e8 04 3c 02 00               	calll	0x464301 <.text+0x63301>
  4406fd: 8b 4d 14                     	movl	0x14(%ebp), %ecx
  440700: 8b f8                        	movl	%eax, %edi
  440702: 89 bc 9c bc 00 00 00         	movl	%edi, 0xbc(%esp,%ebx,4)
  440709: 8b 5c 9c 7c                  	movl	0x7c(%esp,%ebx,4), %ebx
  44070d: 83 c4 04                     	addl	$0x4, %esp
  440710: 8d 04 9d 00 00 00 00         	leal	(,%ebx,4), %eax
  440717: 50                           	pushl	%eax
  440718: 56                           	pushl	%esi
  440719: e8 32 24 fd ff               	calll	0x412b50 <.text+0x11b50>
  44071e: 05 40 01 00 00               	addl	$0x140, %eax            # imm = 0x140
  440723: 50                           	pushl	%eax
  440724: 57                           	pushl	%edi
  440725: e8 a6 34 02 00               	calll	0x463bd0 <.text+0x62bd0>
  44072a: 8b 4d 14                     	movl	0x14(%ebp), %ecx
  44072d: 83 c4 0c                     	addl	$0xc, %esp
  440730: 56                           	pushl	%esi
  440731: e8 1a 24 fd ff               	calll	0x412b50 <.text+0x11b50>
  440736: 68 a0 d9 4c 00               	pushl	$0x4cd9a0               # imm = 0x4CD9A0
  44073b: c7 80 e0 10 00 00 00 00 00 00	movl	$0x0, 0x10e0(%eax)
  440745: ff 15 0c 32 4a 00            	calll	*0x4a320c
  44074b: 85 db                        	testl	%ebx, %ebx
  44074d: 8b 5c 24 28                  	movl	0x28(%esp), %ebx
  440751: 7e 7e                        	jle	0x4407d1 <.text+0x3f7d1>
  440753: 8b 4c 9c 78                  	movl	0x78(%esp,%ebx,4), %ecx
  440757: 89 7c 24 1c                  	movl	%edi, 0x1c(%esp)
  44075b: 89 4c 24 38                  	movl	%ecx, 0x38(%esp)
  44075f: 90                           	nop
  440760: 83 3f 00                     	cmpl	$0x0, (%edi)
  440763: 74 5e                        	je	0x4407c3 <.text+0x3f7c3>
  440765: 8b 17                        	movl	(%edi), %edx
  440767: 8b 42 30                     	movl	0x30(%edx), %eax
  44076a: 83 f8 03                     	cmpl	$0x3, %eax
  44076d: 7c 0a                        	jl	0x440779 <.text+0x3f779>
  44076f: 83 f8 05                     	cmpl	$0x5, %eax
  440772: 7e 4f                        	jle	0x4407c3 <.text+0x3f7c3>
  440774: 83 f8 07                     	cmpl	$0x7, %eax
  440777: 74 4a                        	je	0x4407c3 <.text+0x3f7c3>
  440779: 68 98 00 00 00               	pushl	$0x98
  44077e: e8 ee 32 02 00               	calll	0x463a71 <.text+0x62a71>
  440783: 8b f8                        	movl	%eax, %edi
  440785: 83 c4 04                     	addl	$0x4, %esp
  440788: 85 ff                        	testl	%edi, %edi
  44078a: 74 12                        	je	0x44079e <.text+0x3f79e>
  44078c: 68 98 00 00 00               	pushl	$0x98
  440791: 6a 00                        	pushl	$0x0
  440793: 57                           	pushl	%edi
  440794: e8 77 3b 02 00               	calll	0x464310 <.text+0x63310>
  440799: 83 c4 0c                     	addl	$0xc, %esp
  44079c: eb 02                        	jmp	0x4407a0 <.text+0x3f7a0>
  44079e: 33 ff                        	xorl	%edi, %edi
  4407a0: 8b 44 24 1c                  	movl	0x1c(%esp), %eax
  4407a4: 8b 08                        	movl	(%eax), %ecx
  4407a6: 51                           	pushl	%ecx
  4407a7: 8b cf                        	movl	%edi, %ecx
  4407a9: e8 52 a1 fc ff               	calll	0x40a900 <.text+0x9900>
  4407ae: 8b 4d 14                     	movl	0x14(%ebp), %ecx
  4407b1: 57                           	pushl	%edi
  4407b2: 56                           	pushl	%esi
  4407b3: e8 98 23 fd ff               	calll	0x412b50 <.text+0x11b50>
  4407b8: 8b c8                        	movl	%eax, %ecx
  4407ba: e8 01 26 fd ff               	calll	0x412dc0 <.text+0x11dc0>
  4407bf: 8b 7c 24 1c                  	movl	0x1c(%esp), %edi
  4407c3: 83 c7 04                     	addl	$0x4, %edi
  4407c6: 83 6c 24 38 01               	subl	$0x1, 0x38(%esp)
  4407cb: 89 7c 24 1c                  	movl	%edi, 0x1c(%esp)
  4407cf: 75 8f                        	jne	0x440760 <.text+0x3f760>
  4407d1: 8b 4d 14                     	movl	0x14(%ebp), %ecx
  4407d4: 56                           	pushl	%esi
  4407d5: e8 76 23 fd ff               	calll	0x412b50 <.text+0x11b50>
  4407da: 8b 90 10 01 00 00            	movl	0x110(%eax), %edx
  4407e0: 8b 80 14 01 00 00            	movl	0x114(%eax), %eax
  4407e6: 8b 4d 14                     	movl	0x14(%ebp), %ecx
  4407e9: 56                           	pushl	%esi
  4407ea: 89 94 dc 60 03 00 00         	movl	%edx, 0x360(%esp,%ebx,8)
  4407f1: 89 84 dc 64 03 00 00         	movl	%eax, 0x364(%esp,%ebx,8)
  4407f8: e8 53 23 fd ff               	calll	0x412b50 <.text+0x11b50>
  4407fd: 8b 88 18 01 00 00            	movl	0x118(%eax), %ecx
  440803: 8b 90 1c 01 00 00            	movl	0x11c(%eax), %edx
  440809: 89 8c dc dc 03 00 00         	movl	%ecx, 0x3dc(%esp,%ebx,8)
  440810: 8b 4d 14                     	movl	0x14(%ebp), %ecx
  440813: 56                           	pushl	%esi
  440814: 89 94 dc e4 03 00 00         	movl	%edx, 0x3e4(%esp,%ebx,8)
  44081b: e8 30 23 fd ff               	calll	0x412b50 <.text+0x11b50>
  440820: 8a 80 31 11 00 00            	movb	0x1131(%eax), %al
  440826: 8b 4d 14                     	movl	0x14(%ebp), %ecx
  440829: 6a 00                        	pushl	$0x0
  44082b: 56                           	pushl	%esi
  44082c: 88 84 1c a4 a4 00 00         	movb	%al, 0xa4a4(%esp,%ebx)
  440833: e8 18 23 fd ff               	calll	0x412b50 <.text+0x11b50>
  440838: 8b c8                        	movl	%eax, %ecx
  44083a: e8 41 53 fd ff               	calll	0x415b80 <.text+0x14b80>
  44083f: 8b 7c 24 28                  	movl	0x28(%esp), %edi
  440843: 8b 5c 24 34                  	movl	0x34(%esp), %ebx
  440847: 83 c7 01                     	addl	$0x1, %edi
  44084a: 3b 3d 78 c3 56 00            	cmpl	0x56c378, %edi
  440850: 89 7c 24 28                  	movl	%edi, 0x28(%esp)
  440854: 0f 8c 36 fe ff ff            	jl	0x440690 <.text+0x3f690>
  44085a: 6a 01                        	pushl	$0x1
  44085c: 8b cb                        	movl	%ebx, %ecx
  44085e: e8 1d 53 fd ff               	calll	0x415b80 <.text+0x14b80>
  440863: 8b cb                        	movl	%ebx, %ecx
  440865: e8 26 50 fd ff               	calll	0x415890 <.text+0x14890>
  44086a: 8b 9d 40 01 08 00            	movl	0x80140(%ebp), %ebx
  440870: 8b b5 3c 01 08 00            	movl	0x8013c(%ebp), %esi
  440876: 85 f6                        	testl	%esi, %esi
  440878: 8d 43 ff                     	leal	-0x1(%ebx), %eax
  44087b: 89 44 24 18                  	movl	%eax, 0x18(%esp)
  44087f: 74 0c                        	je	0x44088d <.text+0x3f88d>
  440881: 85 c0                        	testl	%eax, %eax
  440883: 7d 08                        	jge	0x44088d <.text+0x3f88d>
  440885: c7 44 24 18 e7 03 00 00      	movl	$0x3e7, 0x18(%esp)      # imm = 0x3E7
  44088d: 33 ff                        	xorl	%edi, %edi
  44088f: 68 40 9c 00 00               	pushl	$0x9c40                 # imm = 0x9C40
  440894: 8d 8c 24 60 08 00 00         	leal	0x860(%esp), %ecx
  44089b: 57                           	pushl	%edi
  44089c: 51                           	pushl	%ecx
  44089d: 89 7c 24 3c                  	movl	%edi, 0x3c(%esp)
  4408a1: e8 6a 3a 02 00               	calll	0x464310 <.text+0x63310>
  4408a6: 8b d6                        	movl	%esi, %edx
  4408a8: 83 c4 0c                     	addl	$0xc, %esp
  4408ab: 8d b4 24 64 08 00 00         	leal	0x864(%esp), %esi
  4408b2: 8d 04 d2                     	leal	(%edx,%edx,8), %eax
  4408b5: 8b 8c 85 64 01 08 00         	movl	0x80164(%ebp,%eax,4), %ecx
  4408bc: 85 c9                        	testl	%ecx, %ecx
  4408be: 8d 44 85 00                  	leal	(%ebp,%eax,4), %eax
  4408c2: 89 4e 04                     	movl	%ecx, 0x4(%esi)
  4408c5: 7e 2e                        	jle	0x4408f5 <.text+0x3f8f5>
  4408c7: 8b 88 60 01 08 00            	movl	0x80160(%eax), %ecx
  4408cd: 89 0e                        	movl	%ecx, (%esi)
  4408cf: 83 b8 7c 01 08 00 00         	cmpl	$0x0, 0x8017c(%eax)
  4408d6: 74 18                        	je	0x4408f0 <.text+0x3f8f0>
  4408d8: 83 b8 80 01 08 00 10         	cmpl	$0x10, 0x80180(%eax)
  4408df: 72 08                        	jb	0x4408e9 <.text+0x3f8e9>
  4408e1: 8b 80 6c 01 08 00            	movl	0x8016c(%eax), %eax
  4408e7: eb 09                        	jmp	0x4408f2 <.text+0x3f8f2>
  4408e9: 05 6c 01 08 00               	addl	$0x8016c, %eax          # imm = 0x8016C
  4408ee: eb 02                        	jmp	0x4408f2 <.text+0x3f8f2>
  4408f0: 33 c0                        	xorl	%eax, %eax
  4408f2: 89 46 18                     	movl	%eax, 0x18(%esi)
  4408f5: 3b d3                        	cmpl	%ebx, %edx
  4408f7: 74 23                        	je	0x44091c <.text+0x3f91c>
  4408f9: 3b 54 24 18                  	cmpl	0x18(%esp), %edx
  4408fd: 74 1d                        	je	0x44091c <.text+0x3f91c>
  4408ff: 83 c2 01                     	addl	$0x1, %edx
  440902: 81 fa e8 03 00 00            	cmpl	$0x3e8, %edx            # imm = 0x3E8
  440908: 7c 02                        	jl	0x44090c <.text+0x3f90c>
  44090a: 33 d2                        	xorl	%edx, %edx
  44090c: 83 c7 01                     	addl	$0x1, %edi
  44090f: 83 c6 28                     	addl	$0x28, %esi
  440912: 81 ff e8 03 00 00            	cmpl	$0x3e8, %edi            # imm = 0x3E8
  440918: 7c 98                        	jl	0x4408b2 <.text+0x3f8b2>
  44091a: eb 04                        	jmp	0x440920 <.text+0x3f920>
  44091c: 89 7c 24 30                  	movl	%edi, 0x30(%esp)
  440920: 80 7d 68 00                  	cmpb	$0x0, 0x68(%ebp)
  440924: 8b 54 24 30                  	movl	0x30(%esp), %edx
  440928: 89 54 24 24                  	movl	%edx, 0x24(%esp)
  44092c: 0f 84 07 01 00 00            	je	0x440a39 <.text+0x3fa39>
  440932: 8b 44 24 34                  	movl	0x34(%esp), %eax
  440936: c6 05 dd 70 55 00 01         	movb	$0x1, 0x5570dd
  44093d: 8b b0 2c 01 00 00            	movl	0x12c(%eax), %esi
  440943: 3b b0 24 01 00 00            	cmpl	0x124(%eax), %esi
  440949: 8b da                        	movl	%edx, %ebx
  44094b: 89 74 24 38                  	movl	%esi, 0x38(%esp)
  44094f: 7e 67                        	jle	0x4409b8 <.text+0x3f9b8>
  440951: 8d 3c 92                     	leal	(%edx,%edx,4), %edi
  440954: 8d bc fc 78 08 00 00         	leal	0x878(%esp,%edi,8), %edi
  44095b: eb 07                        	jmp	0x440964 <.text+0x3f964>
  44095d: 8d 49 00                     	leal	(%ecx), %ecx
  440960: 8b 54 24 30                  	movl	0x30(%esp), %edx
  440964: 85 db                        	testl	%ebx, %ebx
  440966: 7c 54                        	jl	0x4409bc <.text+0x3f9bc>
  440968: 3b da                        	cmpl	%edx, %ebx
  44096a: 7f 50                        	jg	0x4409bc <.text+0x3f9bc>
  44096c: 8b 54 24 48                  	movl	0x48(%esp), %edx
  440970: 6a 00                        	pushl	$0x0
  440972: 8d 83 10 27 00 00            	leal	0x2710(%ebx), %eax
  440978: 50                           	pushl	%eax
  440979: 8b 44 24 58                  	movl	0x58(%esp), %eax
  44097d: 8d 8c 24 00 01 00 00         	leal	0x100(%esp), %ecx
  440984: 51                           	pushl	%ecx
  440985: 8b 4c 24 40                  	movl	0x40(%esp), %ecx
  440989: 52                           	pushl	%edx
  44098a: 50                           	pushl	%eax
  44098b: 51                           	pushl	%ecx
  44098c: 6a 00                        	pushl	$0x0
  44098e: 55                           	pushl	%ebp
  44098f: 8d 54 24 58                  	leal	0x58(%esp), %edx
  440993: 52                           	pushl	%edx
  440994: 8d 47 e4                     	leal	-0x1c(%edi), %eax
  440997: 50                           	pushl	%eax
  440998: e8 73 f0 ff ff               	calll	0x43fa10 <.text+0x3ea10>
  44099d: 2b 37                        	subl	(%edi), %esi
  44099f: 8b 4c 24 5c                  	movl	0x5c(%esp), %ecx
  4409a3: 83 c4 28                     	addl	$0x28, %esp
  4409a6: 83 eb 01                     	subl	$0x1, %ebx
  4409a9: 83 ef 28                     	subl	$0x28, %edi
  4409ac: 3b b1 24 01 00 00            	cmpl	0x124(%ecx), %esi
  4409b2: 89 74 24 38                  	movl	%esi, 0x38(%esp)
  4409b6: 7f a8                        	jg	0x440960 <.text+0x3f960>
  4409b8: 8b 54 24 30                  	movl	0x30(%esp), %edx
  4409bc: 8b 7c 24 34                  	movl	0x34(%esp), %edi
  4409c0: 8b 8f 2c 01 00 00            	movl	0x12c(%edi), %ecx
  4409c6: 8b bf 24 01 00 00            	movl	0x124(%edi), %edi
  4409cc: 8d 34 92                     	leal	(%edx,%edx,4), %esi
  4409cf: 8d b4 f4 78 08 00 00         	leal	0x878(%esp,%esi,8), %esi
  4409d6: 8b d9                        	movl	%ecx, %ebx
  4409d8: 2b 1e                        	subl	(%esi), %ebx
  4409da: 8b c2                        	movl	%edx, %eax
  4409dc: 3b df                        	cmpl	%edi, %ebx
  4409de: 7e 30                        	jle	0x440a10 <.text+0x3fa10>
  4409e0: 85 c0                        	testl	%eax, %eax
  4409e2: 89 44 24 24                  	movl	%eax, 0x24(%esp)
  4409e6: 7c 20                        	jl	0x440a08 <.text+0x3fa08>
  4409e8: 3b c2                        	cmpl	%edx, %eax
  4409ea: 7f 1c                        	jg	0x440a08 <.text+0x3fa08>
  4409ec: 80 7e e4 01                  	cmpb	$0x1, -0x1c(%esi)
  4409f0: 75 1e                        	jne	0x440a10 <.text+0x3fa10>
  4409f2: 2b 0e                        	subl	(%esi), %ecx
  4409f4: 83 ee 28                     	subl	$0x28, %esi
  4409f7: 8b d9                        	movl	%ecx, %ebx
  4409f9: 2b 1e                        	subl	(%esi), %ebx
  4409fb: 89 44 24 24                  	movl	%eax, 0x24(%esp)
  4409ff: 83 e8 01                     	subl	$0x1, %eax
  440a02: 3b df                        	cmpl	%edi, %ebx
  440a04: 7f da                        	jg	0x4409e0 <.text+0x3f9e0>
  440a06: eb 08                        	jmp	0x440a10 <.text+0x3fa10>
  440a08: c7 44 24 24 00 00 00 00      	movl	$0x0, 0x24(%esp)
  440a10: 8d b4 24 5c 08 00 00         	leal	0x85c(%esp), %esi
  440a17: bf e8 03 00 00               	movl	$0x3e8, %edi            # imm = 0x3E8
  440a1c: 8d 64 24 00                  	leal	(%esp), %esp
  440a20: 8d 94 24 f8 00 00 00         	leal	0xf8(%esp), %edx
  440a27: 52                           	pushl	%edx
  440a28: 56                           	pushl	%esi
  440a29: e8 e2 cc ff ff               	calll	0x43d710 <.text+0x3c710>
  440a2e: 83 c4 08                     	addl	$0x8, %esp
  440a31: 83 c6 28                     	addl	$0x28, %esi
  440a34: 83 ef 01                     	subl	$0x1, %edi
  440a37: 75 e7                        	jne	0x440a20 <.text+0x3fa20>
  440a39: 83 bd 30 91 08 00 00         	cmpl	$0x0, 0x89130(%ebp)
  440a40: 8b 44 24 24                  	movl	0x24(%esp), %eax
  440a44: 89 44 24 18                  	movl	%eax, 0x18(%esp)
  440a48: c7 44 24 2c 00 00 00 00      	movl	$0x0, 0x2c(%esp)
  440a50: 75 27                        	jne	0x440a79 <.text+0x3fa79>
  440a52: 6a 00                        	pushl	$0x0
  440a54: 8d 8c 24 fc 00 00 00         	leal	0xfc(%esp), %ecx
  440a5b: e8 c0 7a fd ff               	calll	0x418520 <.text+0x17520>
  440a60: 8b 0d 28 71 55 00            	movl	0x557128, %ecx
  440a66: 8b 15 2c 71 55 00            	movl	0x55712c, %edx
  440a6c: 51                           	pushl	%ecx
  440a6d: 52                           	pushl	%edx
  440a6e: 6a 00                        	pushl	$0x0
  440a70: 6a 00                        	pushl	$0x0
  440a72: 8b c8                        	movl	%eax, %ecx
  440a74: e8 d7 78 fd ff               	calll	0x418350 <.text+0x17350>
  440a79: 6a 01                        	pushl	$0x1
  440a7b: 8d 8c 24 fc 00 00 00         	leal	0xfc(%esp), %ecx
  440a82: e8 99 7a fd ff               	calll	0x418520 <.text+0x17520>
  440a87: 83 ec 44                     	subl	$0x44, %esp
  440a8a: 8b fc                        	movl	%esp, %edi
  440a8c: b9 11 00 00 00               	movl	$0x11, %ecx
  440a91: be 28 c1 56 00               	movl	$0x56c128, %esi         # imm = 0x56C128
  440a96: f3 a5                        	rep		movsl	(%esi), %es:(%edi)
  440a98: 8b c8                        	movl	%eax, %ecx
  440a9a: e8 81 79 fd ff               	calll	0x418420 <.text+0x17420>
  440a9f: 6a 02                        	pushl	$0x2
  440aa1: 8d 8c 24 fc 00 00 00         	leal	0xfc(%esp), %ecx
  440aa8: e8 73 7a fd ff               	calll	0x418520 <.text+0x17520>
  440aad: 8b 0d f4 c1 56 00            	movl	0x56c1f4, %ecx
  440ab3: 6a 01                        	pushl	$0x1
  440ab5: 51                           	pushl	%ecx
  440ab6: 8b c8                        	movl	%eax, %ecx
  440ab8: e8 23 79 fd ff               	calll	0x4183e0 <.text+0x173e0>
  440abd: 6a 03                        	pushl	$0x3
  440abf: 8d 8c 24 fc 00 00 00         	leal	0xfc(%esp), %ecx
  440ac6: e8 55 7a fd ff               	calll	0x418520 <.text+0x17520>
  440acb: 8b 15 f8 c1 56 00            	movl	0x56c1f8, %edx
  440ad1: 6a 01                        	pushl	$0x1
  440ad3: 52                           	pushl	%edx
  440ad4: 8b c8                        	movl	%eax, %ecx
  440ad6: e8 05 79 fd ff               	calll	0x4183e0 <.text+0x173e0>
  440adb: 6a 02                        	pushl	$0x2
  440add: 8d 8c 24 fc 00 00 00         	leal	0xfc(%esp), %ecx
  440ae4: e8 37 7a fd ff               	calll	0x418520 <.text+0x17520>
  440ae9: 6a 01                        	pushl	$0x1
  440aeb: 6a 26                        	pushl	$0x26
  440aed: 8b c8                        	movl	%eax, %ecx
  440aef: e8 ec 78 fd ff               	calll	0x4183e0 <.text+0x173e0>
  440af4: 6a 03                        	pushl	$0x3
  440af6: 8d 8c 24 fc 00 00 00         	leal	0xfc(%esp), %ecx
  440afd: e8 1e 7a fd ff               	calll	0x418520 <.text+0x17520>
  440b02: 6a 01                        	pushl	$0x1
  440b04: 6a 28                        	pushl	$0x28
  440b06: 8b c8                        	movl	%eax, %ecx
  440b08: e8 d3 78 fd ff               	calll	0x4183e0 <.text+0x173e0>
  440b0d: 6a 04                        	pushl	$0x4
  440b0f: 8d 8c 24 fc 00 00 00         	leal	0xfc(%esp), %ecx
  440b16: e8 05 7a fd ff               	calll	0x418520 <.text+0x17520>
  440b1b: 6a 04                        	pushl	$0x4
  440b1d: 8b c8                        	movl	%eax, %ecx
  440b1f: e8 5c 78 fd ff               	calll	0x418380 <.text+0x17380>
  440b24: 6a 05                        	pushl	$0x5
  440b26: 8d 8c 24 fc 00 00 00         	leal	0xfc(%esp), %ecx
  440b2d: e8 ee 79 fd ff               	calll	0x418520 <.text+0x17520>
  440b32: 6a 05                        	pushl	$0x5
  440b34: 8b c8                        	movl	%eax, %ecx
  440b36: e8 45 78 fd ff               	calll	0x418380 <.text+0x17380>
  440b3b: 6a 07                        	pushl	$0x7
  440b3d: 8d 8c 24 fc 00 00 00         	leal	0xfc(%esp), %ecx
  440b44: e8 d7 79 fd ff               	calll	0x418520 <.text+0x17520>
  440b49: 8b f0                        	movl	%eax, %esi
  440b4b: 6a 02                        	pushl	$0x2
  440b4d: 8b ce                        	movl	%esi, %ecx
  440b4f: e8 2c 78 fd ff               	calll	0x418380 <.text+0x17380>
  440b54: 8b 85 ac 91 08 00            	movl	0x891ac(%ebp), %eax
  440b5a: 8d 5d 18                     	leal	0x18(%ebp), %ebx
  440b5d: 50                           	pushl	%eax
  440b5e: 8b cb                        	movl	%ebx, %ecx
  440b60: e8 bb cd 01 00               	calll	0x45d920 <.text+0x5c920>
  440b65: 85 c0                        	testl	%eax, %eax
  440b67: 74 06                        	je	0x440b6f <.text+0x3fb6f>
  440b69: 89 86 e4 00 00 00            	movl	%eax, 0xe4(%esi)
  440b6f: 6a 07                        	pushl	$0x7
  440b71: 8d 8c 24 fc 00 00 00         	leal	0xfc(%esp), %ecx
  440b78: e8 a3 79 fd ff               	calll	0x418520 <.text+0x17520>
  440b7d: 83 ec 44                     	subl	$0x44, %esp
  440b80: 8b fc                        	movl	%esp, %edi
  440b82: b9 11 00 00 00               	movl	$0x11, %ecx
  440b87: be b0 c1 56 00               	movl	$0x56c1b0, %esi         # imm = 0x56C1B0
  440b8c: f3 a5                        	rep		movsl	(%esi), %es:(%edi)
  440b8e: 8b c8                        	movl	%eax, %ecx
  440b90: 89 84 24 90 00 00 00         	movl	%eax, 0x90(%esp)
  440b97: e8 84 78 fd ff               	calll	0x418420 <.text+0x17420>
  440b9c: 8b 8d ac 91 08 00            	movl	0x891ac(%ebp), %ecx
  440ba2: 51                           	pushl	%ecx
  440ba3: 8b cb                        	movl	%ebx, %ecx
  440ba5: e8 76 cd 01 00               	calll	0x45d920 <.text+0x5c920>
  440baa: 85 c0                        	testl	%eax, %eax
  440bac: 74 0a                        	je	0x440bb8 <.text+0x3fbb8>
  440bae: 8b 54 24 4c                  	movl	0x4c(%esp), %edx
  440bb2: 89 82 e4 00 00 00            	movl	%eax, 0xe4(%edx)
  440bb8: 6a 08                        	pushl	$0x8
  440bba: 8d 8c 24 fc 00 00 00         	leal	0xfc(%esp), %ecx
  440bc1: e8 5a 79 fd ff               	calll	0x418520 <.text+0x17520>
  440bc6: 8b f0                        	movl	%eax, %esi
  440bc8: a1 fc c1 56 00               	movl	0x56c1fc, %eax
  440bcd: 50                           	pushl	%eax
  440bce: 8b ce                        	movl	%esi, %ecx
  440bd0: e8 cb 77 fd ff               	calll	0x4183a0 <.text+0x173a0>
  440bd5: 6a 09                        	pushl	$0x9
  440bd7: 8d 8c 24 fc 00 00 00         	leal	0xfc(%esp), %ecx
  440bde: c6 86 9c 00 00 00 01         	movb	$0x1, 0x9c(%esi)
  440be5: e8 36 79 fd ff               	calll	0x418520 <.text+0x17520>
  440bea: 8b 0d 00 c2 56 00            	movl	0x56c200, %ecx
  440bf0: 8b f0                        	movl	%eax, %esi
  440bf2: 51                           	pushl	%ecx
  440bf3: 8b ce                        	movl	%esi, %ecx
  440bf5: e8 a6 77 fd ff               	calll	0x4183a0 <.text+0x173a0>
  440bfa: 6a 00                        	pushl	$0x0
  440bfc: 8d 54 24 30                  	leal	0x30(%esp), %edx
  440c00: 52                           	pushl	%edx
  440c01: 8d 84 24 64 04 00 00         	leal	0x464(%esp), %eax
  440c08: 50                           	pushl	%eax
  440c09: 8d 8c 24 04 01 00 00         	leal	0x104(%esp), %ecx
  440c10: 51                           	pushl	%ecx
  440c11: 55                           	pushl	%ebp
  440c12: 6a 00                        	pushl	$0x0
  440c14: 6a 01                        	pushl	$0x1
  440c16: 8d 95 20 90 08 00            	leal	0x89020(%ebp), %edx
  440c1c: 6a 02                        	pushl	$0x2
  440c1e: 52                           	pushl	%edx
  440c1f: c6 86 9c 00 00 00 01         	movb	$0x1, 0x9c(%esi)
  440c26: e8 95 2d ff ff               	calll	0x4339c0 <.text+0x329c0>
  440c2b: 6a 00                        	pushl	$0x0
  440c2d: 89 84 24 9c 00 00 00         	movl	%eax, 0x9c(%esp)
  440c34: 8d 44 24 54                  	leal	0x54(%esp), %eax
  440c38: 50                           	pushl	%eax
  440c39: 8d 8c 24 88 04 00 00         	leal	0x488(%esp), %ecx
  440c40: 51                           	pushl	%ecx
  440c41: 8d 94 24 28 01 00 00         	leal	0x128(%esp), %edx
  440c48: 52                           	pushl	%edx
  440c49: 55                           	pushl	%ebp
  440c4a: 6a 00                        	pushl	$0x0
  440c4c: 6a 01                        	pushl	$0x1
  440c4e: 8d 85 98 8f 08 00            	leal	0x88f98(%ebp), %eax
  440c54: 6a 03                        	pushl	$0x3
  440c56: 50                           	pushl	%eax
  440c57: e8 64 2d ff ff               	calll	0x4339c0 <.text+0x329c0>
  440c5c: 83 c4 48                     	addl	$0x48, %esp
  440c5f: 6a 00                        	pushl	$0x0
  440c61: 8d 4c 24 30                  	leal	0x30(%esp), %ecx
  440c65: 51                           	pushl	%ecx
  440c66: 8b d8                        	movl	%eax, %ebx
  440c68: 8d 94 24 64 04 00 00         	leal	0x464(%esp), %edx
  440c6f: 52                           	pushl	%edx
  440c70: 8d 84 24 04 01 00 00         	leal	0x104(%esp), %eax
  440c77: 50                           	pushl	%eax
  440c78: 55                           	pushl	%ebp
  440c79: 6a 00                        	pushl	$0x0
  440c7b: 6a 01                        	pushl	$0x1
  440c7d: 8d 8d ec 90 08 00            	leal	0x890ec(%ebp), %ecx
  440c83: 6a 0a                        	pushl	$0xa
  440c85: 51                           	pushl	%ecx
  440c86: 89 9c 24 90 00 00 00         	movl	%ebx, 0x90(%esp)
  440c8d: e8 2e 2d ff ff               	calll	0x4339c0 <.text+0x329c0>
  440c92: 6a 00                        	pushl	$0x0
  440c94: 8d 54 24 54                  	leal	0x54(%esp), %edx
  440c98: 52                           	pushl	%edx
  440c99: 89 84 24 90 00 00 00         	movl	%eax, 0x90(%esp)
  440ca0: 8d 84 24 88 04 00 00         	leal	0x488(%esp), %eax
  440ca7: 50                           	pushl	%eax
  440ca8: 8d 8c 24 28 01 00 00         	leal	0x128(%esp), %ecx
  440caf: 51                           	pushl	%ecx
  440cb0: 55                           	pushl	%ebp
  440cb1: 6a 00                        	pushl	$0x0
  440cb3: 6a 01                        	pushl	$0x1
  440cb5: 8d 95 a8 90 08 00            	leal	0x890a8(%ebp), %edx
  440cbb: 6a 0b                        	pushl	$0xb
  440cbd: 52                           	pushl	%edx
  440cbe: e8 fd 2c ff ff               	calll	0x4339c0 <.text+0x329c0>
  440cc3: 83 c4 48                     	addl	$0x48, %esp
  440cc6: 89 44 24 58                  	movl	%eax, 0x58(%esp)
  440cca: 6a 00                        	pushl	$0x0
  440ccc: 8d 44 24 30                  	leal	0x30(%esp), %eax
  440cd0: 50                           	pushl	%eax
  440cd1: 8d 8c 24 64 04 00 00         	leal	0x464(%esp), %ecx
  440cd8: 51                           	pushl	%ecx
  440cd9: 8d 94 24 04 01 00 00         	leal	0x104(%esp), %edx
  440ce0: 52                           	pushl	%edx
  440ce1: 55                           	pushl	%ebp
  440ce2: 6a 00                        	pushl	$0x0
  440ce4: 6a 01                        	pushl	$0x1
  440ce6: 8d 85 64 90 08 00            	leal	0x89064(%ebp), %eax
  440cec: 6a 08                        	pushl	$0x8
  440cee: 50                           	pushl	%eax
  440cef: e8 cc 2c ff ff               	calll	0x4339c0 <.text+0x329c0>
  440cf4: 6a 00                        	pushl	$0x0
  440cf6: 8d 4c 24 54                  	leal	0x54(%esp), %ecx
  440cfa: 51                           	pushl	%ecx
  440cfb: 8d 94 24 88 04 00 00         	leal	0x488(%esp), %edx
  440d02: 52                           	pushl	%edx
  440d03: 89 84 24 a0 00 00 00         	movl	%eax, 0xa0(%esp)
  440d0a: 8d 84 24 28 01 00 00         	leal	0x128(%esp), %eax
  440d11: 50                           	pushl	%eax
  440d12: 55                           	pushl	%ebp
  440d13: 6a 00                        	pushl	$0x0
  440d15: 6a 01                        	pushl	$0x1
  440d17: 8d 8d dc 8f 08 00            	leal	0x88fdc(%ebp), %ecx
  440d1d: 6a 09                        	pushl	$0x9
  440d1f: 51                           	pushl	%ecx
  440d20: e8 9b 2c ff ff               	calll	0x4339c0 <.text+0x329c0>
  440d25: 83 c4 48                     	addl	$0x48, %esp
  440d28: 6a 00                        	pushl	$0x0
  440d2a: 8d 54 24 30                  	leal	0x30(%esp), %edx
  440d2e: 52                           	pushl	%edx
  440d2f: 89 44 24 40                  	movl	%eax, 0x40(%esp)
  440d33: 8d 84 24 64 04 00 00         	leal	0x464(%esp), %eax
  440d3a: 50                           	pushl	%eax
  440d3b: 8d 8c 24 04 01 00 00         	leal	0x104(%esp), %ecx
  440d42: 51                           	pushl	%ecx
  440d43: 55                           	pushl	%ebp
  440d44: 6a 00                        	pushl	$0x0
  440d46: 6a 01                        	pushl	$0x1
  440d48: 8d 95 b8 91 08 00            	leal	0x891b8(%ebp), %edx
  440d4e: 6a 0c                        	pushl	$0xc
  440d50: 52                           	pushl	%edx
  440d51: e8 6a 2c ff ff               	calll	0x4339c0 <.text+0x329c0>
  440d56: 83 c4 24                     	addl	$0x24, %esp
  440d59: 85 c0                        	testl	%eax, %eax
  440d5b: 74 07                        	je	0x440d64 <.text+0x3fd64>
  440d5d: c6 80 9c 00 00 00 01         	movb	$0x1, 0x9c(%eax)
  440d64: 6a 00                        	pushl	$0x0
  440d66: 8d 44 24 30                  	leal	0x30(%esp), %eax
  440d6a: 50                           	pushl	%eax
  440d6b: 8d 8c 24 64 04 00 00         	leal	0x464(%esp), %ecx
  440d72: 51                           	pushl	%ecx
  440d73: 8d 94 24 04 01 00 00         	leal	0x104(%esp), %edx
  440d7a: 52                           	pushl	%edx
  440d7b: 55                           	pushl	%ebp
  440d7c: 6a 00                        	pushl	$0x0
  440d7e: 6a 01                        	pushl	$0x1
  440d80: 8d 85 fc 91 08 00            	leal	0x891fc(%ebp), %eax
  440d86: 6a 0d                        	pushl	$0xd
  440d88: 50                           	pushl	%eax
  440d89: e8 32 2c ff ff               	calll	0x4339c0 <.text+0x329c0>
  440d8e: 83 c4 24                     	addl	$0x24, %esp
  440d91: 85 c0                        	testl	%eax, %eax
  440d93: 89 44 24 28                  	movl	%eax, 0x28(%esp)
  440d97: 74 07                        	je	0x440da0 <.text+0x3fda0>
  440d99: c6 80 9c 00 00 00 01         	movb	$0x1, 0x9c(%eax)
  440da0: 6a 00                        	pushl	$0x0
  440da2: 8d 4c 24 30                  	leal	0x30(%esp), %ecx
  440da6: 51                           	pushl	%ecx
  440da7: 8d 94 24 64 04 00 00         	leal	0x464(%esp), %edx
  440dae: 52                           	pushl	%edx
  440daf: 8d 84 24 04 01 00 00         	leal	0x104(%esp), %eax
  440db6: 50                           	pushl	%eax
  440db7: 55                           	pushl	%ebp
  440db8: 6a 00                        	pushl	$0x0
  440dba: 6a 01                        	pushl	$0x1
  440dbc: 8d 85 30 91 08 00            	leal	0x89130(%ebp), %eax
  440dc2: 6a 06                        	pushl	$0x6
  440dc4: 50                           	pushl	%eax
  440dc5: e8 f6 2b ff ff               	calll	0x4339c0 <.text+0x329c0>
  440dca: 8b b5 40 92 08 00            	movl	0x89240(%ebp), %esi
  440dd0: 83 c4 24                     	addl	$0x24, %esp
  440dd3: 85 f6                        	testl	%esi, %esi
  440dd5: 74 6c                        	je	0x440e43 <.text+0x3fe43>
  440dd7: bf 0e 00 00 00               	movl	$0xe, %edi
  440ddc: 8d 64 24 00                  	leal	(%esp), %esp
  440de0: f7 46 40 00 00 00 80         	testl	$0x80000000, 0x40(%esi) # imm = 0x80000000
  440de7: 74 4d                        	je	0x440e36 <.text+0x3fe36>
  440de9: 6a 00                        	pushl	$0x0
  440deb: 8d 4c 24 30                  	leal	0x30(%esp), %ecx
  440def: 51                           	pushl	%ecx
  440df0: 8d 94 24 64 04 00 00         	leal	0x464(%esp), %edx
  440df7: 52                           	pushl	%edx
  440df8: 8d 84 24 04 01 00 00         	leal	0x104(%esp), %eax
  440dff: 50                           	pushl	%eax
  440e00: 55                           	pushl	%ebp
  440e01: 6a 00                        	pushl	$0x0
  440e03: 6a 01                        	pushl	$0x1
  440e05: 57                           	pushl	%edi
  440e06: 56                           	pushl	%esi
  440e07: e8 b4 2b ff ff               	calll	0x4339c0 <.text+0x329c0>
  440e0c: 83 c4 24                     	addl	$0x24, %esp
  440e0f: 85 c0                        	testl	%eax, %eax
  440e11: 74 23                        	je	0x440e36 <.text+0x3fe36>
  440e13: 8b 4e 44                     	movl	0x44(%esi), %ecx
  440e16: 85 c9                        	testl	%ecx, %ecx
  440e18: 74 0c                        	je	0x440e26 <.text+0x3fe26>
  440e1a: 80 39 00                     	cmpb	$0x0, (%ecx)
  440e1d: 75 07                        	jne	0x440e26 <.text+0x3fe26>
  440e1f: c7 40 08 05 00 00 00         	movl	$0x5, 0x8(%eax)
  440e26: 80 be 90 00 00 00 01         	cmpb	$0x1, 0x90(%esi)
  440e2d: 75 07                        	jne	0x440e36 <.text+0x3fe36>
  440e2f: c6 80 cb 00 00 00 01         	movb	$0x1, 0xcb(%eax)
  440e36: 8b b6 98 00 00 00            	movl	0x98(%esi), %esi
  440e3c: 83 c7 01                     	addl	$0x1, %edi
  440e3f: 85 f6                        	testl	%esi, %esi
  440e41: 75 9d                        	jne	0x440de0 <.text+0x3fde0>
  440e43: 85 db                        	testl	%ebx, %ebx
  440e45: c7 44 24 20 00 00 00 00      	movl	$0x0, 0x20(%esp)
  440e4d: b9 01 00 00 00               	movl	$0x1, %ecx
  440e52: 74 06                        	je	0x440e5a <.text+0x3fe5a>
  440e54: 89 8b b8 00 00 00            	movl	%ecx, 0xb8(%ebx)
  440e5a: 8b 44 24 58                  	movl	0x58(%esp), %eax
  440e5e: 85 c0                        	testl	%eax, %eax
  440e60: 74 06                        	je	0x440e68 <.text+0x3fe68>
  440e62: 89 88 b8 00 00 00            	movl	%ecx, 0xb8(%eax)
  440e68: 8b 44 24 38                  	movl	0x38(%esp), %eax
  440e6c: 85 c0                        	testl	%eax, %eax
  440e6e: 74 06                        	je	0x440e76 <.text+0x3fe76>
  440e70: 89 88 b8 00 00 00            	movl	%ecx, 0xb8(%eax)
  440e76: c7 44 24 4c ff ff ff ff      	movl	$0xffffffff, 0x4c(%esp) # imm = 0xFFFFFFFF
  440e7e: 8b 44 24 20                  	movl	0x20(%esp), %eax
  440e82: 8a 1d 9b c8 56 00            	movb	0x56c89b, %bl
  440e88: 8b 4c 24 18                  	movl	0x18(%esp), %ecx
  440e8c: 8b 54 24 24                  	movl	0x24(%esp), %edx
  440e90: 33 f6                        	xorl	%esi, %esi
  440e92: 3b c6                        	cmpl	%esi, %eax
  440e94: c6 44 24 17 00               	movb	$0x0, 0x17(%esp)
  440e99: 8d 7e 01                     	leal	0x1(%esi), %edi
  440e9c: 74 36                        	je	0x440ed4 <.text+0x3fed4>
  440e9e: 7d 06                        	jge	0x440ea6 <.text+0x3fea6>
  440ea0: 2b cf                        	subl	%edi, %ecx
  440ea2: 03 c7                        	addl	%edi, %eax
  440ea4: eb 06                        	jmp	0x440eac <.text+0x3feac>
  440ea6: 7e 04                        	jle	0x440eac <.text+0x3feac>
  440ea8: 03 cf                        	addl	%edi, %ecx
  440eaa: 2b c7                        	subl	%edi, %eax
  440eac: 80 fb 01                     	cmpb	$0x1, %bl
  440eaf: 75 04                        	jne	0x440eb5 <.text+0x3feb5>
  440eb1: 3b ca                        	cmpl	%edx, %ecx
  440eb3: 7f 12                        	jg	0x440ec7 <.text+0x3fec7>
  440eb5: 3b ce                        	cmpl	%esi, %ecx
  440eb7: 7d 02                        	jge	0x440ebb <.text+0x3febb>
  440eb9: 33 c9                        	xorl	%ecx, %ecx
  440ebb: 3b ca                        	cmpl	%edx, %ecx
  440ebd: 7e 02                        	jle	0x440ec1 <.text+0x3fec1>
  440ebf: 8b ca                        	movl	%edx, %ecx
  440ec1: 3b c6                        	cmpl	%esi, %eax
  440ec3: 75 d9                        	jne	0x440e9e <.text+0x3fe9e>
  440ec5: eb 05                        	jmp	0x440ecc <.text+0x3fecc>
  440ec7: c6 44 24 17 00               	movb	$0x0, 0x17(%esp)
  440ecc: 89 4c 24 18                  	movl	%ecx, 0x18(%esp)
  440ed0: 89 44 24 20                  	movl	%eax, 0x20(%esp)
  440ed4: 3b ce                        	cmpl	%esi, %ecx
  440ed6: 8b 44 24 74                  	movl	0x74(%esp), %eax
  440eda: 7f 28                        	jg	0x440f04 <.text+0x3ff04>
  440edc: 3b c6                        	cmpl	%esi, %eax
  440ede: 74 06                        	je	0x440ee6 <.text+0x3fee6>
  440ee0: 89 b8 b8 00 00 00            	movl	%edi, 0xb8(%eax)
  440ee6: 8b 44 24 64                  	movl	0x64(%esp), %eax
  440eea: 3b c6                        	cmpl	%esi, %eax
  440eec: 74 06                        	je	0x440ef4 <.text+0x3fef4>
  440eee: 89 b8 b8 00 00 00            	movl	%edi, 0xb8(%eax)
  440ef4: 8b 44 24 70                  	movl	0x70(%esp), %eax
  440ef8: 3b c6                        	cmpl	%esi, %eax
  440efa: 74 2e                        	je	0x440f2a <.text+0x3ff2a>
  440efc: 89 b8 b8 00 00 00            	movl	%edi, 0xb8(%eax)
  440f02: eb 26                        	jmp	0x440f2a <.text+0x3ff2a>
  440f04: 3b c6                        	cmpl	%esi, %eax
  440f06: 74 06                        	je	0x440f0e <.text+0x3ff0e>
  440f08: 89 b0 b8 00 00 00            	movl	%esi, 0xb8(%eax)
  440f0e: 8b 44 24 64                  	movl	0x64(%esp), %eax
  440f12: 3b c6                        	cmpl	%esi, %eax
  440f14: 74 06                        	je	0x440f1c <.text+0x3ff1c>
  440f16: 89 b0 b8 00 00 00            	movl	%esi, 0xb8(%eax)
  440f1c: 8b 44 24 70                  	movl	0x70(%esp), %eax
  440f20: 3b c6                        	cmpl	%esi, %eax
  440f22: 74 06                        	je	0x440f2a <.text+0x3ff2a>
  440f24: 89 b0 b8 00 00 00            	movl	%esi, 0xb8(%eax)
  440f2a: 80 7d 68 01                  	cmpb	$0x1, 0x68(%ebp)
  440f2e: 75 56                        	jne	0x440f86 <.text+0x3ff86>
  440f30: 3b ca                        	cmpl	%edx, %ecx
  440f32: 8b 44 24 6c                  	movl	0x6c(%esp), %eax
  440f36: 7c 28                        	jl	0x440f60 <.text+0x3ff60>
  440f38: 3b c6                        	cmpl	%esi, %eax
  440f3a: 74 06                        	je	0x440f42 <.text+0x3ff42>
  440f3c: 89 b8 b8 00 00 00            	movl	%edi, 0xb8(%eax)
  440f42: 8b 44 24 58                  	movl	0x58(%esp), %eax
  440f46: 3b c6                        	cmpl	%esi, %eax
  440f48: 74 06                        	je	0x440f50 <.text+0x3ff50>
  440f4a: 89 b8 b8 00 00 00            	movl	%edi, 0xb8(%eax)
  440f50: 8b 44 24 38                  	movl	0x38(%esp), %eax
  440f54: 3b c6                        	cmpl	%esi, %eax
  440f56: 74 2e                        	je	0x440f86 <.text+0x3ff86>
  440f58: 89 b8 b8 00 00 00            	movl	%edi, 0xb8(%eax)
  440f5e: eb 26                        	jmp	0x440f86 <.text+0x3ff86>
  440f60: 3b c6                        	cmpl	%esi, %eax
  440f62: 74 06                        	je	0x440f6a <.text+0x3ff6a>
  440f64: 89 b0 b8 00 00 00            	movl	%esi, 0xb8(%eax)
  440f6a: 8b 44 24 58                  	movl	0x58(%esp), %eax
  440f6e: 3b c6                        	cmpl	%esi, %eax
  440f70: 74 06                        	je	0x440f78 <.text+0x3ff78>
  440f72: 89 b0 b8 00 00 00            	movl	%esi, 0xb8(%eax)
  440f78: 8b 44 24 38                  	movl	0x38(%esp), %eax
  440f7c: 3b c6                        	cmpl	%esi, %eax
  440f7e: 74 06                        	je	0x440f86 <.text+0x3ff86>
  440f80: 89 b0 b8 00 00 00            	movl	%esi, 0xb8(%eax)
  440f86: 80 3d 9b c8 56 00 01         	cmpb	$0x1, 0x56c89b
  440f8d: 75 0a                        	jne	0x440f99 <.text+0x3ff99>
  440f8f: 3b ca                        	cmpl	%edx, %ecx
  440f91: 0f 8f a6 06 00 00            	jg	0x44163d <.text+0x4063d>
  440f97: 33 f6                        	xorl	%esi, %esi
  440f99: 39 74 24 28                  	cmpl	%esi, 0x28(%esp)
  440f9d: 0f 84 30 01 00 00            	je	0x4410d3 <.text+0x400d3>
  440fa3: 8b 44 24 28                  	movl	0x28(%esp), %eax
  440fa7: 8b b0 d4 00 00 00            	movl	0xd4(%eax), %esi
  440fad: 85 f6                        	testl	%esi, %esi
  440faf: 89 74 24 54                  	movl	%esi, 0x54(%esp)
  440fb3: 75 22                        	jne	0x440fd7 <.text+0x3ffd7>
  440fb5: 8b b0 d8 00 00 00            	movl	0xd8(%eax), %esi
  440fbb: 85 f6                        	testl	%esi, %esi
  440fbd: 75 18                        	jne	0x440fd7 <.text+0x3ffd7>
  440fbf: 8b 80 dc 00 00 00            	movl	0xdc(%eax), %eax
  440fc5: 85 c0                        	testl	%eax, %eax
  440fc7: 0f 84 06 01 00 00            	je	0x4410d3 <.text+0x400d3>
  440fcd: 8b f0                        	movl	%eax, %esi
  440fcf: 85 f6                        	testl	%esi, %esi
  440fd1: 0f 84 fc 00 00 00            	je	0x4410d3 <.text+0x400d3>
  440fd7: 8b 4e 10                     	movl	0x10(%esi), %ecx
  440fda: db 44 24 24                  	fildl	0x24(%esp)
  440fde: 8b 9d 08 92 08 00            	movl	0x89208(%ebp), %ebx
  440fe4: 8b c1                        	movl	%ecx, %eax
  440fe6: 99                           	cltd
  440fe7: d9 5c 24 44                  	fstps	0x44(%esp)
  440feb: 2b c2                        	subl	%edx, %eax
  440fed: 8b f8                        	movl	%eax, %edi
  440fef: 8b c1                        	movl	%ecx, %eax
  440ff1: 8b 8d 10 92 08 00            	movl	0x89210(%ebp), %ecx
  440ff7: 99                           	cltd
  440ff8: 2b c2                        	subl	%edx, %eax
  440ffa: d1 f8                        	sarl	%eax
  440ffc: 03 c0                        	addl	%eax, %eax
  440ffe: 2b c8                        	subl	%eax, %ecx
  441000: 2b cb                        	subl	%ebx, %ecx
  441002: 89 4c 24 1c                  	movl	%ecx, 0x1c(%esp)
  441006: 8b 76 08                     	movl	0x8(%esi), %esi
  441009: db 44 24 1c                  	fildl	0x1c(%esp)
  44100d: d1 ff                        	sarl	%edi
  44100f: 03 f7                        	addl	%edi, %esi
  441011: d9 5c 24 1c                  	fstps	0x1c(%esp)
  441015: db 44 24 18                  	fildl	0x18(%esp)
  441019: d9 44 24 44                  	flds	0x44(%esp)
  44101d: d9 c0                        	fld	%st(0)
  44101f: de fa                        	fdivrp	%st, %st(2)
  441021: d9 44 24 1c                  	flds	0x1c(%esp)
  441025: d9 c0                        	fld	%st(0)
  441027: de cb                        	fmulp	%st, %st(3)
  441029: d9 ca                        	fxch	%st(2)
  44102b: e8 70 3c 02 00               	calll	0x464ca0 <.text+0x63ca0>
  441030: 8b 54 24 18                  	movl	0x18(%esp), %edx
  441034: 83 c2 01                     	addl	$0x1, %edx
  441037: 89 54 24 44                  	movl	%edx, 0x44(%esp)
  44103b: da 7c 24 44                  	fidivrl	0x44(%esp)
  44103f: 8b d8                        	movl	%eax, %ebx
  441041: 03 9d 08 92 08 00            	addl	0x89208(%ebp), %ebx
  441047: 03 df                        	addl	%edi, %ebx
  441049: de c9                        	fmulp	%st, %st(1)
  44104b: e8 50 3c 02 00               	calll	0x464ca0 <.text+0x63ca0>
  441050: 03 85 08 92 08 00            	addl	0x89208(%ebp), %eax
  441056: 03 c7                        	addl	%edi, %eax
  441058: 3b f3                        	cmpl	%ebx, %esi
  44105a: 7c 04                        	jl	0x441060 <.text+0x40060>
  44105c: 3b f0                        	cmpl	%eax, %esi
  44105e: 7c 24                        	jl	0x441084 <.text+0x40084>
  441060: 8b 44 24 18                  	movl	0x18(%esp), %eax
  441064: 85 c0                        	testl	%eax, %eax
  441066: 75 0a                        	jne	0x441072 <.text+0x40072>
  441068: 8b b5 08 92 08 00            	movl	0x89208(%ebp), %esi
  44106e: 03 f7                        	addl	%edi, %esi
  441070: eb 12                        	jmp	0x441084 <.text+0x40084>
  441072: 3b 44 24 24                  	cmpl	0x24(%esp), %eax
  441076: 75 0a                        	jne	0x441082 <.text+0x40082>
  441078: 8b b5 10 92 08 00            	movl	0x89210(%ebp), %esi
  44107e: 2b f7                        	subl	%edi, %esi
  441080: eb 02                        	jmp	0x441084 <.text+0x40084>
  441082: 8b f3                        	movl	%ebx, %esi
  441084: 8b 85 08 92 08 00            	movl	0x89208(%ebp), %eax
  44108a: 03 c7                        	addl	%edi, %eax
  44108c: 3b f0                        	cmpl	%eax, %esi
  44108e: 7d 02                        	jge	0x441092 <.text+0x40092>
  441090: 8b f0                        	movl	%eax, %esi
  441092: 8b 85 10 92 08 00            	movl	0x89210(%ebp), %eax
  441098: 2b c7                        	subl	%edi, %eax
  44109a: 3b f0                        	cmpl	%eax, %esi
  44109c: 7e 02                        	jle	0x4410a0 <.text+0x400a0>
  44109e: 8b f0                        	movl	%eax, %esi
  4410a0: 8b 44 24 54                  	movl	0x54(%esp), %eax
  4410a4: 85 c0                        	testl	%eax, %eax
  4410a6: 74 07                        	je	0x4410af <.text+0x400af>
  4410a8: 8b ce                        	movl	%esi, %ecx
  4410aa: 2b cf                        	subl	%edi, %ecx
  4410ac: 89 48 08                     	movl	%ecx, 0x8(%eax)
  4410af: 8b 4c 24 28                  	movl	0x28(%esp), %ecx
  4410b3: 8b 81 d8 00 00 00            	movl	0xd8(%ecx), %eax
  4410b9: 85 c0                        	testl	%eax, %eax
  4410bb: 74 07                        	je	0x4410c4 <.text+0x400c4>
  4410bd: 8b d6                        	movl	%esi, %edx
  4410bf: 2b d7                        	subl	%edi, %edx
  4410c1: 89 50 08                     	movl	%edx, 0x8(%eax)
  4410c4: 8b 81 dc 00 00 00            	movl	0xdc(%ecx), %eax
  4410ca: 85 c0                        	testl	%eax, %eax
  4410cc: 74 05                        	je	0x4410d3 <.text+0x400d3>
  4410ce: 2b f7                        	subl	%edi, %esi
  4410d0: 89 70 08                     	movl	%esi, 0x8(%eax)
  4410d3: a1 78 c3 56 00               	movl	0x56c378, %eax
  4410d8: 85 c0                        	testl	%eax, %eax
  4410da: 7e 13                        	jle	0x4410ef <.text+0x400ef>
  4410dc: 50                           	pushl	%eax
  4410dd: 8d 84 24 c0 a4 00 00         	leal	0xa4c0(%esp), %eax
  4410e4: 6a 00                        	pushl	$0x0
  4410e6: 50                           	pushl	%eax
  4410e7: e8 24 32 02 00               	calll	0x464310 <.text+0x63310>
  4410ec: 83 c4 0c                     	addl	$0xc, %esp
  4410ef: 8b 74 24 18                  	movl	0x18(%esp), %esi
  4410f3: 85 f6                        	testl	%esi, %esi
  4410f5: c6 05 dd 70 55 00 01         	movb	$0x1, 0x5570dd
  4410fc: c7 44 24 1c 00 00 00 00      	movl	$0x0, 0x1c(%esp)
  441104: 7e 23                        	jle	0x441129 <.text+0x40129>
  441106: 8d bc 24 5c 08 00 00         	leal	0x85c(%esp), %edi
  44110d: 8b de                        	movl	%esi, %ebx
  44110f: 90                           	nop
  441110: 8d 8c 24 f8 00 00 00         	leal	0xf8(%esp), %ecx
  441117: 51                           	pushl	%ecx
  441118: 57                           	pushl	%edi
  441119: e8 f2 c5 ff ff               	calll	0x43d710 <.text+0x3c710>
  44111e: 83 c4 08                     	addl	$0x8, %esp
  441121: 83 c7 28                     	addl	$0x28, %edi
  441124: 83 eb 01                     	subl	$0x1, %ebx
  441127: 75 e7                        	jne	0x441110 <.text+0x40110>
  441129: 8b 44 24 18                  	movl	0x18(%esp), %eax
  44112d: 3b 44 24 30                  	cmpl	0x30(%esp), %eax
  441131: 8b 5c 24 34                  	movl	0x34(%esp), %ebx
  441135: 0f 8f 27 01 00 00            	jg	0x441262 <.text+0x40262>
  44113b: 8d 3c 80                     	leal	(%eax,%eax,4), %edi
  44113e: 8d bc fc 5c 08 00 00         	leal	0x85c(%esp,%edi,8), %edi
  441145: 8a 45 68                     	movb	0x68(%ebp), %al
  441148: 84 c0                        	testb	%al, %al
  44114a: 74 18                        	je	0x441164 <.text+0x40164>
  44114c: 8b 93 2c 01 00 00            	movl	0x12c(%ebx), %edx
  441152: 2b 93 24 01 00 00            	subl	0x124(%ebx), %edx
  441158: 39 54 24 1c                  	cmpl	%edx, 0x1c(%esp)
  44115c: 0f 8f 00 01 00 00            	jg	0x441262 <.text+0x40262>
  441162: eb 0a                        	jmp	0x44116e <.text+0x4016e>
  441164: 3b 74 24 18                  	cmpl	0x18(%esp), %esi
  441168: 0f 85 f4 00 00 00            	jne	0x441262 <.text+0x40262>
  44116e: 85 f6                        	testl	%esi, %esi
  441170: 0f 8c d5 00 00 00            	jl	0x44124b <.text+0x4024b>
  441176: 3b 74 24 30                  	cmpl	0x30(%esp), %esi
  44117a: 0f 8f cb 00 00 00            	jg	0x44124b <.text+0x4024b>
  441180: 3c 01                        	cmpb	$0x1, %al
  441182: 0f 85 90 00 00 00            	jne	0x441218 <.text+0x40218>
  441188: 8b 54 24 48                  	movl	0x48(%esp), %edx
  44118c: 6a 00                        	pushl	$0x0
  44118e: 8d 86 10 27 00 00            	leal	0x2710(%esi), %eax
  441194: 50                           	pushl	%eax
  441195: 8b 44 24 58                  	movl	0x58(%esp), %eax
  441199: 8d 8c 24 00 01 00 00         	leal	0x100(%esp), %ecx
  4411a0: 51                           	pushl	%ecx
  4411a1: 52                           	pushl	%edx
  4411a2: 50                           	pushl	%eax
  4411a3: 53                           	pushl	%ebx
  4411a4: 8d 8c 24 d4 a4 00 00         	leal	0xa4d4(%esp), %ecx
  4411ab: 51                           	pushl	%ecx
  4411ac: 55                           	pushl	%ebp
  4411ad: 8d 54 24 3c                  	leal	0x3c(%esp), %edx
  4411b1: 52                           	pushl	%edx
  4411b2: 57                           	pushl	%edi
  4411b3: e8 58 e8 ff ff               	calll	0x43fa10 <.text+0x3ea10>
  4411b8: 8b 8b d0 00 00 00            	movl	0xd0(%ebx), %ecx
  4411be: 8b 93 d4 00 00 00            	movl	0xd4(%ebx), %edx
  4411c4: 83 c4 18                     	addl	$0x18, %esp
  4411c7: 8b c4                        	movl	%esp, %eax
  4411c9: 89 08                        	movl	%ecx, (%eax)
  4411cb: 8b 8b d8 00 00 00            	movl	0xd8(%ebx), %ecx
  4411d1: 89 50 04                     	movl	%edx, 0x4(%eax)
  4411d4: 8b 93 dc 00 00 00            	movl	0xdc(%ebx), %edx
  4411da: 89 48 08                     	movl	%ecx, 0x8(%eax)
  4411dd: 8b 8b 20 01 00 00            	movl	0x120(%ebx), %ecx
  4411e3: 89 50 0c                     	movl	%edx, 0xc(%eax)
  4411e6: 8b 93 24 01 00 00            	movl	0x124(%ebx), %edx
  4411ec: 83 ec 10                     	subl	$0x10, %esp
  4411ef: 8b c4                        	movl	%esp, %eax
  4411f1: 89 08                        	movl	%ecx, (%eax)
  4411f3: 8b 8b 28 01 00 00            	movl	0x128(%ebx), %ecx
  4411f9: 89 50 04                     	movl	%edx, 0x4(%eax)
  4411fc: 8b 93 2c 01 00 00            	movl	0x12c(%ebx), %edx
  441202: 89 48 08                     	movl	%ecx, 0x8(%eax)
  441205: 89 50 0c                     	movl	%edx, 0xc(%eax)
  441208: 8d 84 24 18 01 00 00         	leal	0x118(%esp), %eax
  44120f: 50                           	pushl	%eax
  441210: 57                           	pushl	%edi
  441211: e8 2a 2d ff ff               	calll	0x433f40 <.text+0x32f40>
  441216: eb 30                        	jmp	0x441248 <.text+0x40248>
  441218: 8b 44 24 48                  	movl	0x48(%esp), %eax
  44121c: 6a 01                        	pushl	$0x1
  44121e: 8d 8e 10 27 00 00            	leal	0x2710(%esi), %ecx
  441224: 51                           	pushl	%ecx
  441225: 8b 4c 24 58                  	movl	0x58(%esp), %ecx
  441229: 8d 94 24 00 01 00 00         	leal	0x100(%esp), %edx
  441230: 52                           	pushl	%edx
  441231: 50                           	pushl	%eax
  441232: 51                           	pushl	%ecx
  441233: 53                           	pushl	%ebx
  441234: 8d 94 24 d4 a4 00 00         	leal	0xa4d4(%esp), %edx
  44123b: 52                           	pushl	%edx
  44123c: 55                           	pushl	%ebp
  44123d: 8d 44 24 3c                  	leal	0x3c(%esp), %eax
  441241: 50                           	pushl	%eax
  441242: 57                           	pushl	%edi
  441243: e8 c8 e7 ff ff               	calll	0x43fa10 <.text+0x3ea10>
  441248: 83 c4 28                     	addl	$0x28, %esp
  44124b: 8b 4f 1c                     	movl	0x1c(%edi), %ecx
  44124e: 01 4c 24 1c                  	addl	%ecx, 0x1c(%esp)
  441252: 83 c6 01                     	addl	$0x1, %esi
  441255: 83 c7 28                     	addl	$0x28, %edi
  441258: 3b 74 24 30                  	cmpl	0x30(%esp), %esi
  44125c: 0f 8e e3 fe ff ff            	jle	0x441145 <.text+0x40145>
  441262: 8b 7c 24 30                  	movl	0x30(%esp), %edi
  441266: 3b f7                        	cmpl	%edi, %esi
  441268: 7f 44                        	jg	0x4412ae <.text+0x402ae>
  44126a: 8d 04 b6                     	leal	(%esi,%esi,4), %eax
  44126d: 8d 84 c4 5c 08 00 00         	leal	0x85c(%esp,%eax,8), %eax
  441274: 2b fe                        	subl	%esi, %edi
  441276: 89 44 24 1c                  	movl	%eax, 0x1c(%esp)
  44127a: 83 c7 01                     	addl	$0x1, %edi
  44127d: eb 05                        	jmp	0x441284 <.text+0x40284>
  44127f: 90                           	nop
  441280: 8b 44 24 1c                  	movl	0x1c(%esp), %eax
  441284: 80 38 01                     	cmpb	$0x1, (%eax)
  441287: 75 0a                        	jne	0x441293 <.text+0x40293>
  441289: 8d 94 24 f8 00 00 00         	leal	0xf8(%esp), %edx
  441290: 52                           	pushl	%edx
  441291: eb 08                        	jmp	0x44129b <.text+0x4029b>
  441293: 8d 8c 24 f8 00 00 00         	leal	0xf8(%esp), %ecx
  44129a: 51                           	pushl	%ecx
  44129b: 50                           	pushl	%eax
  44129c: e8 6f c4 ff ff               	calll	0x43d710 <.text+0x3c710>
  4412a1: 83 44 24 24 28               	addl	$0x28, 0x24(%esp)
  4412a6: 83 c4 08                     	addl	$0x8, %esp
  4412a9: 83 ef 01                     	subl	$0x1, %edi
  4412ac: 75 d2                        	jne	0x441280 <.text+0x40280>
  4412ae: 8b cb                        	movl	%ebx, %ecx
  4412b0: e8 1b 1e fd ff               	calll	0x4130d0 <.text+0x120d0>
  4412b5: 80 7d 68 00                  	cmpb	$0x0, 0x68(%ebp)
  4412b9: 75 4b                        	jne	0x441306 <.text+0x40306>
  4412bb: 33 f6                        	xorl	%esi, %esi
  4412bd: 39 35 78 c3 56 00            	cmpl	%esi, 0x56c378
  4412c3: 7e 38                        	jle	0x4412fd <.text+0x402fd>
  4412c5: 8b 3c b5 7c c3 56 00         	movl	0x56c37c(,%esi,4), %edi
  4412cc: 8b 4d 14                     	movl	0x14(%ebp), %ecx
  4412cf: 57                           	pushl	%edi
  4412d0: e8 7b 18 fd ff               	calll	0x412b50 <.text+0x11b50>
  4412d5: 85 c0                        	testl	%eax, %eax
  4412d7: 74 19                        	je	0x4412f2 <.text+0x402f2>
  4412d9: 0f b6 94 34 bc a4 00 00      	movzbl	0xa4bc(%esp,%esi), %edx
  4412e1: 8b 4d 14                     	movl	0x14(%ebp), %ecx
  4412e4: 52                           	pushl	%edx
  4412e5: 57                           	pushl	%edi
  4412e6: e8 65 18 fd ff               	calll	0x412b50 <.text+0x11b50>
  4412eb: 8b c8                        	movl	%eax, %ecx
  4412ed: e8 8e 48 fd ff               	calll	0x415b80 <.text+0x14b80>
  4412f2: 83 c6 01                     	addl	$0x1, %esi
  4412f5: 3b 35 78 c3 56 00            	cmpl	0x56c378, %esi
  4412fb: 7c c8                        	jl	0x4412c5 <.text+0x402c5>
  4412fd: 6a 01                        	pushl	$0x1
  4412ff: 8b cb                        	movl	%ebx, %ecx
  441301: e8 7a 48 fd ff               	calll	0x415b80 <.text+0x14b80>
  441306: c6 05 dd 70 55 00 00         	movb	$0x0, 0x5570dd
  44130d: 8b 4d 14                     	movl	0x14(%ebp), %ecx
  441310: e8 cb 19 fd ff               	calll	0x412ce0 <.text+0x11ce0>
  441315: b9 a0 70 55 00               	movl	$0x5570a0, %ecx         # imm = 0x5570A0
  44131a: e8 b1 4a fe ff               	calll	0x425dd0 <.text+0x24dd0>
  44131f: 8b 44 24 4c                  	movl	0x4c(%esp), %eax
  441323: 83 f8 0c                     	cmpl	$0xc, %eax
  441326: 75 34                        	jne	0x44135c <.text+0x4035c>
  441328: 80 3d 6c 72 55 00 01         	cmpb	$0x1, 0x55726c
  44132f: 75 30                        	jne	0x441361 <.text+0x40361>
  441331: 6a 00                        	pushl	$0x0
  441333: 8d 8c 24 fc 00 00 00         	leal	0xfc(%esp), %ecx
  44133a: c7 84 24 0c 01 00 00 00 00 00 00     	movl	$0x0, 0x10c(%esp)
  441345: c6 84 24 22 01 00 00 01      	movb	$0x1, 0x122(%esp)
  44134d: c6 84 24 23 01 00 00 01      	movb	$0x1, 0x123(%esp)
  441355: e8 26 88 fd ff               	calll	0x419b80 <.text+0x18b80>
  44135a: eb 2c                        	jmp	0x441388 <.text+0x40388>
  44135c: 83 f8 0d                     	cmpl	$0xd, %eax
  44135f: 74 c7                        	je	0x441328 <.text+0x40328>
  441361: 8d 8c 24 f8 00 00 00         	leal	0xf8(%esp), %ecx
  441368: c7 84 24 08 01 00 00 00 00 00 00     	movl	$0x0, 0x108(%esp)
  441373: c6 84 24 1e 01 00 00 01      	movb	$0x1, 0x11e(%esp)
  44137b: c6 84 24 1f 01 00 00 01      	movb	$0x1, 0x11f(%esp)
  441383: e8 88 8a fd ff               	calll	0x419e10 <.text+0x18e10>
  441388: 80 3d 40 c3 56 00 01         	cmpb	$0x1, 0x56c340
  44138f: 8b d8                        	movl	%eax, %ebx
  441391: 89 5c 24 4c                  	movl	%ebx, 0x4c(%esp)
  441395: 0f 84 a2 02 00 00            	je	0x44163d <.text+0x4063d>
  44139b: 80 3d 48 c3 56 00 01         	cmpb	$0x1, 0x56c348
  4413a2: 0f 84 95 02 00 00            	je	0x44163d <.text+0x4063d>
  4413a8: 80 3d 49 c3 56 00 01         	cmpb	$0x1, 0x56c349
  4413af: 0f 84 88 02 00 00            	je	0x44163d <.text+0x4063d>
  4413b5: 80 3d d2 87 55 00 01         	cmpb	$0x1, 0x5587d2
  4413bc: 0f 84 7b 02 00 00            	je	0x44163d <.text+0x4063d>
  4413c2: 83 fb 0d                     	cmpl	$0xd, %ebx
  4413c5: 0f 87 8d 01 00 00            	ja	0x441558 <.text+0x40558>
  4413cb: ff 24 9d 40 18 44 00         	jmpl	*0x441840(,%ebx,4)
  4413d2: 83 6c 24 20 01               	subl	$0x1, 0x20(%esp)
  4413d7: e9 22 02 00 00               	jmp	0x4415fe <.text+0x405fe>
  4413dc: 83 44 24 20 01               	addl	$0x1, 0x20(%esp)
  4413e1: e9 18 02 00 00               	jmp	0x4415fe <.text+0x405fe>
  4413e6: 80 7d 68 00                  	cmpb	$0x0, 0x68(%ebp)
  4413ea: 74 0f                        	je	0x4413fb <.text+0x403fb>
  4413ec: 8b 85 50 92 08 00            	movl	0x89250(%ebp), %eax
  4413f2: 29 44 24 20                  	subl	%eax, 0x20(%esp)
  4413f6: e9 83 fa ff ff               	jmp	0x440e7e <.text+0x3fe7e>
  4413fb: 83 6c 24 20 01               	subl	$0x1, 0x20(%esp)
  441400: e9 79 fa ff ff               	jmp	0x440e7e <.text+0x3fe7e>
  441405: 80 7d 68 00                  	cmpb	$0x0, 0x68(%ebp)
  441409: 74 0f                        	je	0x44141a <.text+0x4041a>
  44140b: 8b 8d 50 92 08 00            	movl	0x89250(%ebp), %ecx
  441411: 01 4c 24 20                  	addl	%ecx, 0x20(%esp)
  441415: e9 64 fa ff ff               	jmp	0x440e7e <.text+0x3fe7e>
  44141a: 83 44 24 20 01               	addl	$0x1, 0x20(%esp)
  44141f: e9 5a fa ff ff               	jmp	0x440e7e <.text+0x3fe7e>
  441424: 8b 44 24 18                  	movl	0x18(%esp), %eax
  441428: f7 d8                        	negl	%eax
  44142a: 89 44 24 20                  	movl	%eax, 0x20(%esp)
  44142e: e9 4b fa ff ff               	jmp	0x440e7e <.text+0x3fe7e>
  441433: 8b 44 24 24                  	movl	0x24(%esp), %eax
  441437: 2b 44 24 18                  	subl	0x18(%esp), %eax
  44143b: 89 44 24 20                  	movl	%eax, 0x20(%esp)
  44143f: e9 3a fa ff ff               	jmp	0x440e7e <.text+0x3fe7e>
  441444: 83 7c 24 28 00               	cmpl	$0x0, 0x28(%esp)
  441449: 0f 84 2f fa ff ff            	je	0x440e7e <.text+0x3fe7e>
  44144f: 8d 54 24 5c                  	leal	0x5c(%esp), %edx
  441453: 52                           	pushl	%edx
  441454: b9 a0 70 55 00               	movl	$0x5570a0, %ecx         # imm = 0x5570A0
  441459: e8 a2 49 fe ff               	calll	0x425e00 <.text+0x24e00>
  44145e: 8b 4c 24 28                  	movl	0x28(%esp), %ecx
  441462: 8b b9 d4 00 00 00            	movl	0xd4(%ecx), %edi
  441468: 85 ff                        	testl	%edi, %edi
  44146a: 74 04                        	je	0x441470 <.text+0x40470>
  44146c: 8b c7                        	movl	%edi, %eax
  44146e: eb 18                        	jmp	0x441488 <.text+0x40488>
  441470: 8b 81 d8 00 00 00            	movl	0xd8(%ecx), %eax
  441476: 85 c0                        	testl	%eax, %eax
  441478: 75 16                        	jne	0x441490 <.text+0x40490>
  44147a: 8b 81 dc 00 00 00            	movl	0xdc(%ecx), %eax
  441480: 85 c0                        	testl	%eax, %eax
  441482: 0f 84 f6 f9 ff ff            	je	0x440e7e <.text+0x3fe7e>
  441488: 85 c0                        	testl	%eax, %eax
  44148a: 0f 84 ee f9 ff ff            	je	0x440e7e <.text+0x3fe7e>
  441490: 8b 70 10                     	movl	0x10(%eax), %esi
  441493: 8b 8d 08 92 08 00            	movl	0x89208(%ebp), %ecx
  441499: 8b c6                        	movl	%esi, %eax
  44149b: 99                           	cltd
  44149c: 2b c2                        	subl	%edx, %eax
  44149e: d1 f8                        	sarl	%eax
  4414a0: 8d 14 08                     	leal	(%eax,%ecx), %edx
  4414a3: 8b 4c 24 60                  	movl	0x60(%esp), %ecx
  4414a7: 3b ca                        	cmpl	%edx, %ecx
  4414a9: 7d 06                        	jge	0x4414b1 <.text+0x404b1>
  4414ab: 8b ca                        	movl	%edx, %ecx
  4414ad: 89 4c 24 60                  	movl	%ecx, 0x60(%esp)
  4414b1: 8b 95 10 92 08 00            	movl	0x89210(%ebp), %edx
  4414b7: 2b d0                        	subl	%eax, %edx
  4414b9: 3b ca                        	cmpl	%edx, %ecx
  4414bb: 7e 06                        	jle	0x4414c3 <.text+0x404c3>
  4414bd: 8b ca                        	movl	%edx, %ecx
  4414bf: 89 4c 24 60                  	movl	%ecx, 0x60(%esp)
  4414c3: 85 ff                        	testl	%edi, %edi
  4414c5: 74 09                        	je	0x4414d0 <.text+0x404d0>
  4414c7: 2b c8                        	subl	%eax, %ecx
  4414c9: 89 4f 08                     	movl	%ecx, 0x8(%edi)
  4414cc: 8b 4c 24 60                  	movl	0x60(%esp), %ecx
  4414d0: 8b 7c 24 28                  	movl	0x28(%esp), %edi
  4414d4: 8b 97 d8 00 00 00            	movl	0xd8(%edi), %edx
  4414da: 85 d2                        	testl	%edx, %edx
  4414dc: 74 09                        	je	0x4414e7 <.text+0x404e7>
  4414de: 2b c8                        	subl	%eax, %ecx
  4414e0: 89 4a 08                     	movl	%ecx, 0x8(%edx)
  4414e3: 8b 4c 24 60                  	movl	0x60(%esp), %ecx
  4414e7: 8b 97 dc 00 00 00            	movl	0xdc(%edi), %edx
  4414ed: 85 d2                        	testl	%edx, %edx
  4414ef: 74 09                        	je	0x4414fa <.text+0x404fa>
  4414f1: 2b c8                        	subl	%eax, %ecx
  4414f3: 89 4a 08                     	movl	%ecx, 0x8(%edx)
  4414f6: 8b 4c 24 60                  	movl	0x60(%esp), %ecx
  4414fa: 8b c6                        	movl	%esi, %eax
  4414fc: 99                           	cltd
  4414fd: 2b c2                        	subl	%edx, %eax
  4414ff: 8b 95 08 92 08 00            	movl	0x89208(%ebp), %edx
  441505: d1 f8                        	sarl	%eax
  441507: f7 d8                        	negl	%eax
  441509: 8d 34 00                     	leal	(%eax,%eax), %esi
  44150c: 2b c2                        	subl	%edx, %eax
  44150e: 03 c1                        	addl	%ecx, %eax
  441510: 2b f2                        	subl	%edx, %esi
  441512: 03 b5 10 92 08 00            	addl	0x89210(%ebp), %esi
  441518: 89 44 24 54                  	movl	%eax, 0x54(%esp)
  44151c: 89 74 24 44                  	movl	%esi, 0x44(%esp)
  441520: db 44 24 54                  	fildl	0x54(%esp)
  441524: da 74 24 44                  	fidivl	0x44(%esp)
  441528: da 4c 24 24                  	fimull	0x24(%esp)
  44152c: e8 6f 37 02 00               	calll	0x464ca0 <.text+0x63ca0>
  441531: 33 d2                        	xorl	%edx, %edx
  441533: 85 c0                        	testl	%eax, %eax
  441535: 0f 9c c2                     	setl	%dl
  441538: 83 ea 01                     	subl	$0x1, %edx
  44153b: 23 c2                        	andl	%edx, %eax
  44153d: 3b 44 24 24                  	cmpl	0x24(%esp), %eax
  441541: 89 44 24 18                  	movl	%eax, 0x18(%esp)
  441545: 0f 8e 33 f9 ff ff            	jle	0x440e7e <.text+0x3fe7e>
  44154b: 8b 44 24 24                  	movl	0x24(%esp), %eax
  44154f: 89 44 24 18                  	movl	%eax, 0x18(%esp)
  441553: e9 26 f9 ff ff               	jmp	0x440e7e <.text+0x3fe7e>
  441558: 80 bc 24 1c 01 00 00 01      	cmpb	$0x1, 0x11c(%esp)
  441560: 75 0a                        	jne	0x44156c <.text+0x4056c>
  441562: c6 44 24 17 00               	movb	$0x0, 0x17(%esp)
  441567: e9 97 00 00 00               	jmp	0x441603 <.text+0x40603>
  44156c: 81 fb 10 27 00 00            	cmpl	$0x2710, %ebx           # imm = 0x2710
  441572: 0f 8c 8b 00 00 00            	jl	0x441603 <.text+0x40603>
  441578: 8b 4c 24 30                  	movl	0x30(%esp), %ecx
  44157c: 81 c1 10 27 00 00            	addl	$0x2710, %ecx           # imm = 0x2710
  441582: 3b d9                        	cmpl	%ecx, %ebx
  441584: 7f 7d                        	jg	0x441603 <.text+0x40603>
  441586: 8b 74 24 40                  	movl	0x40(%esp), %esi
  44158a: 85 f6                        	testl	%esi, %esi
  44158c: 74 0f                        	je	0x44159d <.text+0x4059d>
  44158e: 6a 64                        	pushl	$0x64
  441590: 8b ce                        	movl	%esi, %ecx
  441592: e8 99 be 01 00               	calll	0x45d430 <.text+0x5c430>
  441597: 33 f6                        	xorl	%esi, %esi
  441599: 89 74 24 40                  	movl	%esi, 0x40(%esp)
  44159d: 8d 7d 18                     	leal	0x18(%ebp), %edi
  4415a0: 6a 64                        	pushl	$0x64
  4415a2: 8b cf                        	movl	%edi, %ecx
  4415a4: e8 97 c6 01 00               	calll	0x45dc40 <.text+0x5cc40>
  4415a9: 85 f6                        	testl	%esi, %esi
  4415ab: 75 30                        	jne	0x4415dd <.text+0x405dd>
  4415ad: 80 3d bc c3 56 00 01         	cmpb	$0x1, 0x56c3bc
  4415b4: 8b cf                        	movl	%edi, %ecx
  4415b6: 75 14                        	jne	0x4415cc <.text+0x405cc>
  4415b8: 8b 15 c0 c3 56 00            	movl	0x56c3c0, %edx
  4415be: 52                           	pushl	%edx
  4415bf: e8 dc ec 01 00               	calll	0x4602a0 <.text+0x5f2a0>
  4415c4: 8b f0                        	movl	%eax, %esi
  4415c6: 89 74 24 40                  	movl	%esi, 0x40(%esp)
  4415ca: eb 0d                        	jmp	0x4415d9 <.text+0x405d9>
  4415cc: 6a ff                        	pushl	$-0x1
  4415ce: e8 cd ec 01 00               	calll	0x4602a0 <.text+0x5f2a0>
  4415d3: 8b f0                        	movl	%eax, %esi
  4415d5: 89 44 24 40                  	movl	%eax, 0x40(%esp)
  4415d9: 85 f6                        	testl	%esi, %esi
  4415db: 74 21                        	je	0x4415fe <.text+0x405fe>
  4415dd: 8d 84 9b b0 3c ff ff         	leal	-0xc350(%ebx,%ebx,4), %eax
  4415e4: 8b 84 c4 7c 08 00 00         	movl	0x87c(%esp,%eax,8), %eax
  4415eb: 85 c0                        	testl	%eax, %eax
  4415ed: 74 0f                        	je	0x4415fe <.text+0x405fe>
  4415ef: 50                           	pushl	%eax
  4415f0: 8b ce                        	movl	%esi, %ecx
  4415f2: e8 09 eb 01 00               	calll	0x460100 <.text+0x5f100>
  4415f7: 8b ce                        	movl	%esi, %ecx
  4415f9: e8 e2 ca 01 00               	calll	0x45e0e0 <.text+0x5d0e0>
  4415fe: c6 44 24 17 01               	movb	$0x1, 0x17(%esp)
  441603: 83 fb 01                     	cmpl	$0x1, %ebx
  441606: 75 2a                        	jne	0x441632 <.text+0x40632>
  441608: 8b 4d 14                     	movl	0x14(%ebp), %ecx
  44160b: 85 c9                        	testl	%ecx, %ecx
  44160d: 74 23                        	je	0x441632 <.text+0x40632>
  44160f: 8b 55 0c                     	movl	0xc(%ebp), %edx
  441612: 52                           	pushl	%edx
  441613: e8 38 15 fd ff               	calll	0x412b50 <.text+0x11b50>
  441618: 85 c0                        	testl	%eax, %eax
  44161a: 74 16                        	je	0x441632 <.text+0x40632>
  44161c: 38 98 f8 00 00 00            	cmpb	%bl, 0xf8(%eax)
  441622: 75 0e                        	jne	0x441632 <.text+0x40632>
  441624: 55                           	pushl	%ebp
  441625: e8 46 be ff ff               	calll	0x43d470 <.text+0x3c470>
  44162a: 83 c4 04                     	addl	$0x4, %esp
  44162d: e9 4c f8 ff ff               	jmp	0x440e7e <.text+0x3fe7e>
  441632: 80 7c 24 17 00               	cmpb	$0x0, 0x17(%esp)
  441637: 0f 85 41 f8 ff ff            	jne	0x440e7e <.text+0x3fe7e>
  44163d: 80 3d 11 ca 56 00 00         	cmpb	$0x0, 0x56ca11
  441644: a1 58 72 55 00               	movl	0x557258, %eax
  441649: 8b 0d 70 72 55 00            	movl	0x557270, %ecx
  44164f: a3 5c 72 55 00               	movl	%eax, 0x55725c
  441654: 89 0d 74 72 55 00            	movl	%ecx, 0x557274
  44165a: 75 1c                        	jne	0x441678 <.text+0x40678>
  44165c: 8b 74 24 40                  	movl	0x40(%esp), %esi
  441660: 85 f6                        	testl	%esi, %esi
  441662: 74 14                        	je	0x441678 <.text+0x40678>
  441664: 6a 64                        	pushl	$0x64
  441666: 8b ce                        	movl	%esi, %ecx
  441668: e8 33 be 01 00               	calll	0x45d4a0 <.text+0x5c4a0>
  44166d: 8b 16                        	movl	(%esi), %edx
  44166f: 52                           	pushl	%edx
  441670: 8d 4d 18                     	leal	0x18(%ebp), %ecx
  441673: e8 48 e5 01 00               	calll	0x45fbc0 <.text+0x5ebc0>
  441678: 8b 7c 24 2c                  	movl	0x2c(%esp), %edi
  44167c: 33 f6                        	xorl	%esi, %esi
  44167e: 85 ff                        	testl	%edi, %edi
  441680: 7e 1a                        	jle	0x44169c <.text+0x4069c>
  441682: 8b 84 b4 5c 04 00 00         	movl	0x45c(%esp,%esi,4), %eax
  441689: c6 00 00                     	movb	$0x0, (%eax)
  44168c: 8b 48 34                     	movl	0x34(%eax), %ecx
  44168f: 50                           	pushl	%eax
  441690: e8 db 43 fd ff               	calll	0x415a70 <.text+0x14a70>
  441695: 83 c6 01                     	addl	$0x1, %esi
  441698: 3b f7                        	cmpl	%edi, %esi
  44169a: 7c e6                        	jl	0x441682 <.text+0x40682>
  44169c: 8d b4 24 5c 08 00 00         	leal	0x85c(%esp), %esi
  4416a3: bf e8 03 00 00               	movl	$0x3e8, %edi            # imm = 0x3E8
  4416a8: 80 3e 01                     	cmpb	$0x1, (%esi)
  4416ab: 75 11                        	jne	0x4416be <.text+0x406be>
  4416ad: 8d 84 24 f8 00 00 00         	leal	0xf8(%esp), %eax
  4416b4: 50                           	pushl	%eax
  4416b5: 56                           	pushl	%esi
  4416b6: e8 55 c0 ff ff               	calll	0x43d710 <.text+0x3c710>
  4416bb: 83 c4 08                     	addl	$0x8, %esp
  4416be: 83 c6 28                     	addl	$0x28, %esi
  4416c1: 83 ef 01                     	subl	$0x1, %edi
  4416c4: 75 e2                        	jne	0x4416a8 <.text+0x406a8>
  4416c6: 39 7c 24 34                  	cmpl	%edi, 0x34(%esp)
  4416ca: 0f 84 03 01 00 00            	je	0x4417d3 <.text+0x407d3>
  4416d0: 39 3d 78 c3 56 00            	cmpl	%edi, 0x56c378
  4416d6: 0f 8e e6 00 00 00            	jle	0x4417c2 <.text+0x407c2>
  4416dc: 8b 0c bd 7c c3 56 00         	movl	0x56c37c(,%edi,4), %ecx
  4416e3: 51                           	pushl	%ecx
  4416e4: 8b 4d 14                     	movl	0x14(%ebp), %ecx
  4416e7: e8 64 14 fd ff               	calll	0x412b50 <.text+0x11b50>
  4416ec: 8b f0                        	movl	%eax, %esi
  4416ee: 85 f6                        	testl	%esi, %esi
  4416f0: 0f 84 bd 00 00 00            	je	0x4417b3 <.text+0x407b3>
  4416f6: 80 bc 3c ac a4 00 00 01      	cmpb	$0x1, 0xa4ac(%esp,%edi)
  4416fe: 8b ce                        	movl	%esi, %ecx
  441700: 75 04                        	jne	0x441706 <.text+0x40706>
  441702: 6a 01                        	pushl	$0x1
  441704: eb 02                        	jmp	0x441708 <.text+0x40708>
  441706: 6a 00                        	pushl	$0x0
  441708: e8 73 44 fd ff               	calll	0x415b80 <.text+0x14b80>
  44170d: 8b ce                        	movl	%esi, %ecx
  44170f: e8 7c 42 fd ff               	calll	0x415990 <.text+0x14990>
  441714: 68 a0 d9 4c 00               	pushl	$0x4cd9a0               # imm = 0x4CD9A0
  441719: ff 15 10 32 4a 00            	calll	*0x4a3210
  44171f: 8b 5c bc 78                  	movl	0x78(%esp,%edi,4), %ebx
  441723: 8b 84 bc b8 00 00 00         	movl	0xb8(%esp,%edi,4), %eax
  44172a: 8d 14 9d 00 00 00 00         	leal	(,%ebx,4), %edx
  441731: 52                           	pushl	%edx
  441732: 50                           	pushl	%eax
  441733: 8d 86 40 01 00 00            	leal	0x140(%esi), %eax
  441739: 50                           	pushl	%eax
  44173a: e8 91 24 02 00               	calll	0x463bd0 <.text+0x62bd0>
  44173f: 8b 84 bc c4 00 00 00         	movl	0xc4(%esp,%edi,4), %eax
  441746: 50                           	pushl	%eax
  441747: 89 9e e0 10 00 00            	movl	%ebx, 0x10e0(%esi)
  44174d: e8 aa 2b 02 00               	calll	0x4642fc <.text+0x632fc>
  441752: 83 c4 10                     	addl	$0x10, %esp
  441755: 68 a0 d9 4c 00               	pushl	$0x4cd9a0               # imm = 0x4CD9A0
  44175a: c7 84 bc bc 00 00 00 00 00 00 00     	movl	$0x0, 0xbc(%esp,%edi,4)
  441765: ff 15 0c 32 4a 00            	calll	*0x4a320c
  44176b: 8b ce                        	movl	%esi, %ecx
  44176d: e8 5e 19 fd ff               	calll	0x4130d0 <.text+0x120d0>
  441772: 8b 8c fc 5c 03 00 00         	movl	0x35c(%esp,%edi,8), %ecx
  441779: 8b 94 fc 60 03 00 00         	movl	0x360(%esp,%edi,8), %edx
  441780: 8b 84 fc dc 03 00 00         	movl	0x3dc(%esp,%edi,8), %eax
  441787: 89 8e 10 01 00 00            	movl	%ecx, 0x110(%esi)
  44178d: 8b 8c fc e0 03 00 00         	movl	0x3e0(%esp,%edi,8), %ecx
  441794: 89 96 14 01 00 00            	movl	%edx, 0x114(%esi)
  44179a: 8a 94 3c 9c a4 00 00         	movb	0xa49c(%esp,%edi), %dl
  4417a1: 89 86 18 01 00 00            	movl	%eax, 0x118(%esi)
  4417a7: 89 8e 1c 01 00 00            	movl	%ecx, 0x11c(%esi)
  4417ad: 88 96 31 11 00 00            	movb	%dl, 0x1131(%esi)
  4417b3: 83 c7 01                     	addl	$0x1, %edi
  4417b6: 3b 3d 78 c3 56 00            	cmpl	0x56c378, %edi
  4417bc: 0f 8c 1a ff ff ff            	jl	0x4416dc <.text+0x406dc>
  4417c2: 80 7d 68 00                  	cmpb	$0x0, 0x68(%ebp)
  4417c6: 74 0b                        	je	0x4417d3 <.text+0x407d3>
  4417c8: 8b 4c 24 34                  	movl	0x34(%esp), %ecx
  4417cc: 6a 00                        	pushl	$0x0
  4417ce: e8 ad 43 fd ff               	calll	0x415b80 <.text+0x14b80>
  4417d3: b9 a0 70 55 00               	movl	$0x5570a0, %ecx         # imm = 0x5570A0
  4417d8: e8 f3 45 fe ff               	calll	0x425dd0 <.text+0x24dd0>
  4417dd: 8b 44 24 68                  	movl	0x68(%esp), %eax
  4417e1: 85 c0                        	testl	%eax, %eax
  4417e3: 74 05                        	je	0x4417ea <.text+0x407ea>
  4417e5: a3 ac 70 55 00               	movl	%eax, 0x5570ac
  4417ea: 8a 44 24 3f                  	movb	0x3f(%esp), %al
  4417ee: 8d 8c 24 f8 00 00 00         	leal	0xf8(%esp), %ecx
  4417f5: a2 d0 87 55 00               	movb	%al, 0x5587d0
  4417fa: c7 84 24 d8 a4 00 00 ff ff ff ff     	movl	$0xffffffff, 0xa4d8(%esp) # imm = 0xFFFFFFFF
  441805: c7 84 24 f8 00 00 00 74 39 4a 00     	movl	$0x4a3974, 0xf8(%esp) # imm = 0x4A3974
  441810: e8 1b 83 fd ff               	calll	0x419b30 <.text+0x18b30>
  441815: 8b 8c 24 d0 a4 00 00         	movl	0xa4d0(%esp), %ecx
  44181c: 64 89 0d 00 00 00 00         	movl	%ecx, %fs:0x0
  441823: 59                           	popl	%ecx
  441824: 5f                           	popl	%edi
  441825: 5e                           	popl	%esi
  441826: 5d                           	popl	%ebp
  441827: 5b                           	popl	%ebx
  441828: 8b 8c 24 b8 a4 00 00         	movl	0xa4b8(%esp), %ecx
  44182f: 33 cc                        	xorl	%esp, %ecx
  441831: e8 86 23 02 00               	calll	0x463bbc <.text+0x62bbc>
  441836: 81 c4 c8 a4 00 00            	addl	$0xa4c8, %esp           # imm = 0xA4C8
  44183c: c3                           	retl
  44183d: 8d 49 00                     	leal	(%ecx), %ecx
  441840: 62 15 44 00 58 15            	bound	%edx, 0x15580044
  441846: 44                           	incl	%esp
  441847: 00 d2                        	addb	%dl, %dl
  441849: 13 44 00 dc                  	adcl	-0x24(%eax,%eax), %eax
  44184d: 13 44 00 d2                  	adcl	-0x2e(%eax,%eax), %eax
  441851: 13 44 00 dc                  	adcl	-0x24(%eax,%eax), %eax
  441855: 13 44 00 62                  	adcl	0x62(%eax,%eax), %eax
  441859: 15 44 00 62 15               	adcl	$0x15620044, %eax       # imm = 0x15620044
  44185e: 44                           	incl	%esp
  44185f: 00 e6                        	addb	%ah, %dh
  441861: 13 44 00 05                  	adcl	0x5(%eax,%eax), %eax
  441865: 14 44                        	adcb	$0x44, %al
  441867: 00 24 14                     	addb	%ah, (%esp,%edx)
  44186a: 44                           	incl	%esp
  44186b: 00 33                        	addb	%dh, (%ebx)
  44186d: 14 44                        	adcb	$0x44, %al
  44186f: 00 7e 0e                     	addb	%bh, 0xe(%esi)
  441872: 44                           	incl	%esp
  441873: 00 44 14 44                  	addb	%al, 0x44(%esp,%edx)
  441877: 00 cc                        	addb	%cl, %ah
  441879: cc                           	int3
  44187a: cc                           	int3
  44187b: cc                           	int3
  44187c: cc                           	int3
  44187d: cc                           	int3
  44187e: cc                           	int3
  44187f: cc                           	int3
  441880: 8b 44 24 04                  	movl	0x4(%esp), %eax


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  401e10: 83 ec 18                     	subl	$0x18, %esp
  401e13: 33 c0                        	xorl	%eax, %eax
  401e15: 38 05 4c bf 4c 00            	cmpb	%al, 0x4cbf4c
  401e1b: 56                           	pushl	%esi
  401e1c: 8b 35 a4 33 4a 00            	movl	0x4a33a4, %esi
  401e22: 89 44 24 10                  	movl	%eax, 0x10(%esp)
  401e26: 89 44 24 08                  	movl	%eax, 0x8(%esp)
  401e2a: 89 44 24 0c                  	movl	%eax, 0xc(%esp)
  401e2e: 89 44 24 14                  	movl	%eax, 0x14(%esp)
  401e32: 89 44 24 18                  	movl	%eax, 0x18(%esp)
  401e36: c7 44 24 10 04 02 00 00      	movl	$0x204, 0x10(%esp)      # imm = 0x204
  401e3e: 75 0c                        	jne	0x401e4c <.text+0xe4c>
  401e40: 8d 44 24 08                  	leal	0x8(%esp), %eax
  401e44: 50                           	pushl	%eax
  401e45: 68 02 30 00 00               	pushl	$0x3002                 # imm = 0x3002
  401e4a: eb 12                        	jmp	0x401e5e <.text+0xe5e>
  401e4c: 8d 4c 24 08                  	leal	0x8(%esp), %ecx
  401e50: 51                           	pushl	%ecx
  401e51: c7 44 24 18 4c bf 4c 00      	movl	$0x4cbf4c, 0x18(%esp)   # imm = 0x4CBF4C
  401e59: 68 02 32 00 00               	pushl	$0x3202                 # imm = 0x3202
  401e5e: 68 03 08 00 00               	pushl	$0x803                  # imm = 0x803
  401e63: 6a 00                        	pushl	$0x0
  401e65: ff d6                        	calll	*%esi
  401e67: 85 c0                        	testl	%eax, %eax
  401e69: 75 30                        	jne	0x401e9b <.text+0xe9b>
  401e6b: 8b 44 24 0c                  	movl	0xc(%esp), %eax
  401e6f: 8d 54 24 04                  	leal	0x4(%esp), %edx
  401e73: 52                           	pushl	%edx
  401e74: 6a 02                        	pushl	$0x2
  401e76: 68 08 08 00 00               	pushl	$0x808                  # imm = 0x808
  401e7b: 50                           	pushl	%eax
  401e7c: ff d6                        	calll	*%esi
  401e7e: 85 c0                        	testl	%eax, %eax
  401e80: 75 19                        	jne	0x401e9b <.text+0xe9b>
  401e82: 8b 4c 24 0c                  	movl	0xc(%esp), %ecx
  401e86: 50                           	pushl	%eax
  401e87: 50                           	pushl	%eax
  401e88: 68 04 08 00 00               	pushl	$0x804                  # imm = 0x804
  401e8d: 51                           	pushl	%ecx
  401e8e: ff d6                        	calll	*%esi
  401e90: 85 c0                        	testl	%eax, %eax
  401e92: 75 07                        	jne	0x401e9b <.text+0xe9b>
  401e94: b0 01                        	movb	$0x1, %al
  401e96: 5e                           	popl	%esi
  401e97: 83 c4 18                     	addl	$0x18, %esp
  401e9a: c3                           	retl
  401e9b: 32 c0                        	xorb	%al, %al
  401e9d: 5e                           	popl	%esi
  401e9e: 83 c4 18                     	addl	$0x18, %esp
  401ea1: c3                           	retl
  401ea2: cc                           	int3
  401ea3: cc                           	int3
  401ea4: cc                           	int3
  401ea5: cc                           	int3
  401ea6: cc                           	int3
  401ea7: cc                           	int3
  401ea8: cc                           	int3
  401ea9: cc                           	int3
  401eaa: cc                           	int3
  401eab: cc                           	int3
  401eac: cc                           	int3
  401ead: cc                           	int3
  401eae: cc                           	int3
  401eaf: cc                           	int3
  401eb0: 56                           	pushl	%esi
  401eb1: 8b f1                        	movl	%ecx, %esi
  401eb3: 83 3e 00                     	cmpl	$0x0, (%esi)
  401eb6: 74 3b                        	je	0x401ef3 <.text+0xef3>
  401eb8: 8b 46 28                     	movl	0x28(%esi), %eax
  401ebb: 50                           	pushl	%eax
  401ebc: c6 46 1c 01                  	movb	$0x1, 0x1c(%esi)
  401ec0: ff 15 88 30 4a 00            	calll	*0x4a3088
  401ec6: e8 45 ff ff ff               	calll	0x401e10 <.text+0xe10>
  401ecb: 8b 06                        	movl	(%esi), %eax
  401ecd: 85 c0                        	testl	%eax, %eax
  401ecf: c6 46 1c 01                  	movb	$0x1, 0x1c(%esi)
  401ed3: 74 1e                        	je	0x401ef3 <.text+0xef3>
  401ed5: 68 e8 03 00 00               	pushl	$0x3e8                  # imm = 0x3E8
  401eda: 50                           	pushl	%eax
  401edb: ff 15 80 30 4a 00            	calll	*0x4a3080
  401ee1: 3d 80 00 00 00               	cmpl	$0x80, %eax
  401ee6: 75 0b                        	jne	0x401ef3 <.text+0xef3>
  401ee8: 8b 0e                        	movl	(%esi), %ecx
  401eea: 6a ff                        	pushl	$-0x1
  401eec: 51                           	pushl	%ecx
  401eed: ff 15 7c 30 4a 00            	calll	*0x4a307c
  401ef3: 5e                           	popl	%esi
  401ef4: c3                           	retl
  401ef5: cc                           	int3
  401ef6: cc                           	int3
  401ef7: cc                           	int3
  401ef8: cc                           	int3
  401ef9: cc                           	int3
  401efa: cc                           	int3
  401efb: cc                           	int3
  401efc: cc                           	int3
  401efd: cc                           	int3
  401efe: cc                           	int3
  401eff: cc                           	int3
  401f00: 8b 44 24 04                  	movl	0x4(%esp), %eax
  401f04: 56                           	pushl	%esi
  401f05: 8b f1                        	movl	%ecx, %esi
  401f07: 39 46 2c                     	cmpl	%eax, 0x2c(%esi)
  401f0a: 74 52                        	je	0x401f5e <.text+0xf5e>
  401f0c: 89 46 2c                     	movl	%eax, 0x2c(%esi)
  401f0f: 8b 0d 48 bf 4c 00            	movl	0x4cbf48, %ecx
  401f15: 8b 44 24 0c                  	movl	0xc(%esp), %eax
  401f19: 51                           	pushl	%ecx
  401f1a: a3 64 c0 4c 00               	movl	%eax, 0x4cc064
  401f1f: c6 05 48 c0 4b 00 01         	movb	$0x1, 0x4bc048
  401f26: ff 15 84 30 4a 00            	calll	*0x4a3084
  401f2c: 8b 56 28                     	movl	0x28(%esi), %edx
  401f2f: 52                           	pushl	%edx
  401f30: ff 15 88 30 4a 00            	calll	*0x4a3088
  401f36: a1 48 bf 4c 00               	movl	0x4cbf48, %eax
  401f3b: 68 e8 03 00 00               	pushl	$0x3e8                  # imm = 0x3E8
  401f40: 50                           	pushl	%eax
  401f41: ff 15 80 30 4a 00            	calll	*0x4a3080
  401f47: 3d 02 01 00 00               	cmpl	$0x102, %eax            # imm = 0x102
  401f4c: 75 10                        	jne	0x401f5e <.text+0xf5e>
  401f4e: ff 15 a0 33 4a 00            	calll	*0x4a33a0
  401f54: a3 5c bf 4c 00               	movl	%eax, 0x4cbf5c
  401f59: a3 68 c0 4c 00               	movl	%eax, 0x4cc068
  401f5e: b0 01                        	movb	$0x1, %al
  401f60: 5e                           	popl	%esi
  401f61: c2 08 00                     	retl	$0x8
  401f64: cc                           	int3
  401f65: cc                           	int3
  401f66: cc                           	int3
  401f67: cc                           	int3
  401f68: cc                           	int3
  401f69: cc                           	int3
  401f6a: cc                           	int3
  401f6b: cc                           	int3
  401f6c: cc                           	int3
  401f6d: cc                           	int3
  401f6e: cc                           	int3
  401f6f: cc                           	int3
  401f70: 56                           	pushl	%esi
  401f71: 8b f1                        	movl	%ecx, %esi
  401f73: c7 46 2c 00 00 00 00         	movl	$0x0, 0x2c(%esi)
  401f7a: a1 48 bf 4c 00               	movl	0x4cbf48, %eax
  401f7f: 50                           	pushl	%eax
  401f80: ff 15 84 30 4a 00            	calll	*0x4a3084
  401f86: 8b 4e 28                     	movl	0x28(%esi), %ecx
  401f89: 51                           	pushl	%ecx
  401f8a: ff 15 88 30 4a 00            	calll	*0x4a3088
  401f90: 8b 15 48 bf 4c 00            	movl	0x4cbf48, %edx
  401f96: 68 e8 03 00 00               	pushl	$0x3e8                  # imm = 0x3E8
  401f9b: 52                           	pushl	%edx
  401f9c: ff 15 80 30 4a 00            	calll	*0x4a3080
  401fa2: b0 01                        	movb	$0x1, %al
  401fa4: 5e                           	popl	%esi
  401fa5: c3                           	retl
  401fa6: cc                           	int3
  401fa7: cc                           	int3
  401fa8: cc                           	int3
  401fa9: cc                           	int3
  401faa: cc                           	int3
  401fab: cc                           	int3
  401fac: cc                           	int3
  401fad: cc                           	int3
  401fae: cc                           	int3
  401faf: cc                           	int3
  401fb0: 8b 44 24 04                  	movl	0x4(%esp), %eax
  401fb4: a3 64 c0 4c 00               	movl	%eax, 0x4cc064
  401fb9: c2 04 00                     	retl	$0x4
  401fbc: cc                           	int3
  401fbd: cc                           	int3
  401fbe: cc                           	int3
  401fbf: cc                           	int3
  401fc0: 56                           	pushl	%esi
  401fc1: 33 f6                        	xorl	%esi, %esi
  401fc3: ff 15 a0 33 4a 00            	calll	*0x4a33a0
  401fc9: 8b 4c 24 08                  	movl	0x8(%esp), %ecx
  401fcd: 83 e9 01                     	subl	$0x1, %ecx
  401fd0: 74 15                        	je	0x401fe7 <.text+0xfe7>
  401fd2: 83 e9 01                     	subl	$0x1, %ecx
  401fd5: 75 20                        	jne	0x401ff7 <.text+0xff7>
  401fd7: 8b 0d 5c bf 4c 00            	movl	0x4cbf5c, %ecx
  401fdd: 85 c9                        	testl	%ecx, %ecx
  401fdf: 75 10                        	jne	0x401ff1 <.text+0xff1>
  401fe1: 33 c0                        	xorl	%eax, %eax
  401fe3: 5e                           	popl	%esi
  401fe4: c2 04 00                     	retl	$0x4
  401fe7: 8b 0d 68 c0 4c 00            	movl	0x4cc068, %ecx
  401fed: 85 c9                        	testl	%ecx, %ecx
  401fef: 74 f0                        	je	0x401fe1 <.text+0xfe1>
  401ff1: 2b c1                        	subl	%ecx, %eax
  401ff3: 5e                           	popl	%esi
  401ff4: c2 04 00                     	retl	$0x4
  401ff7: 8b c6                        	movl	%esi, %eax
  401ff9: 5e                           	popl	%esi
  401ffa: c2 04 00                     	retl	$0x4
  401ffd: cc                           	int3
  401ffe: cc                           	int3
  401fff: cc                           	int3
  402000: 8a 44 24 04                  	movb	0x4(%esp), %al
  402004: 3c 01                        	cmpb	$0x1, %al
  402006: 88 41 30                     	movb	%al, 0x30(%ecx)
  402009: 75 0d                        	jne	0x402018 <.text+0x1018>
  40200b: 8b 41 28                     	movl	0x28(%ecx), %eax
  40200e: 89 44 24 04                  	movl	%eax, 0x4(%esp)
  402012: ff 25 88 30 4a 00            	jmpl	*0x4a3088
  402018: 8b 41 2c                     	movl	0x2c(%ecx), %eax
  40201b: 85 c0                        	testl	%eax, %eax
  40201d: 7e 20                        	jle	0x40203f <.text+0x103f>
  40201f: 83 3d 64 c0 4c 00 01         	cmpl	$0x1, 0x4cc064
  402026: 75 17                        	jne	0x40203f <.text+0x103f>
  402028: c7 41 2c 00 00 00 00         	movl	$0x0, 0x2c(%ecx)
  40202f: 8b 15 64 c0 4c 00            	movl	0x4cc064, %edx
  402035: 52                           	pushl	%edx
  402036: 50                           	pushl	%eax
  402037: e8 c4 fe ff ff               	calll	0x401f00 <.text+0xf00>
  40203c: c2 04 00                     	retl	$0x4
  40203f: e8 2c ff ff ff               	calll	0x401f70 <.text+0xf70>


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  402050: 81 ec 64 02 00 00            	subl	$0x264, %esp            # imm = 0x264
  402056: a1 48 ea 4b 00               	movl	0x4bea48, %eax
  40205b: 33 c4                        	xorl	%esp, %eax
  40205d: 89 84 24 60 02 00 00         	movl	%eax, 0x260(%esp)
  402064: 53                           	pushl	%ebx
  402065: 56                           	pushl	%esi
  402066: 57                           	pushl	%edi
  402067: 8b bc 24 74 02 00 00         	movl	0x274(%esp), %edi
  40206e: 8d 84 24 6c 01 00 00         	leal	0x16c(%esp), %eax
  402075: 50                           	pushl	%eax
  402076: 68 00 01 00 00               	pushl	$0x100                  # imm = 0x100
  40207b: 32 db                        	xorb	%bl, %bl
  40207d: ff 15 94 30 4a 00            	calll	*0x4a3094
  402083: 38 9c 24 6c 01 00 00         	cmpb	%bl, 0x16c(%esp)
  40208a: 8d b4 24 6c 01 00 00         	leal	0x16c(%esp), %esi
  402091: 0f 84 99 00 00 00            	je	0x402130 <.text+0x1130>
  402097: 8b 1d 90 30 4a 00            	movl	0x4a3090, %ebx
  40209d: 55                           	pushl	%ebp
  40209e: 8b 2d 58 32 4a 00            	movl	0x4a3258, %ebp
  4020a4: 56                           	pushl	%esi
  4020a5: ff d3                        	calll	*%ebx
  4020a7: 83 f8 05                     	cmpl	$0x5, %eax
  4020aa: 75 37                        	jne	0x4020e3 <.text+0x10e3>
  4020ac: 68 00 02 00 00               	pushl	$0x200                  # imm = 0x200
  4020b1: 68 60 01 00 00               	pushl	$0x160                  # imm = 0x160
  4020b6: 8d 4c 24 18                  	leal	0x18(%esp), %ecx
  4020ba: 51                           	pushl	%ecx
  4020bb: 6a 00                        	pushl	$0x0
  4020bd: 56                           	pushl	%esi
  4020be: ff d5                        	calll	*%ebp
  4020c0: 8b c7                        	movl	%edi, %eax
  4020c2: 8d 50 01                     	leal	0x1(%eax), %edx
  4020c5: 8a 08                        	movb	(%eax), %cl
  4020c7: 83 c0 01                     	addl	$0x1, %eax
  4020ca: 84 c9                        	testb	%cl, %cl
  4020cc: 75 f7                        	jne	0x4020c5 <.text+0x10c5>
  4020ce: 2b c2                        	subl	%edx, %eax
  4020d0: 50                           	pushl	%eax
  4020d1: 8d 54 24 20                  	leal	0x20(%esp), %edx
  4020d5: 57                           	pushl	%edi
  4020d6: 52                           	pushl	%edx
  4020d7: e8 11 8b 07 00               	calll	0x47abed <.text+0x79bed>
  4020dc: 83 c4 0c                     	addl	$0xc, %esp
  4020df: 85 c0                        	testl	%eax, %eax
  4020e1: 74 32                        	je	0x402115 <.text+0x1115>
  4020e3: 6a 00                        	pushl	$0x0
  4020e5: 56                           	pushl	%esi
  4020e6: e8 65 25 06 00               	calll	0x464650 <.text+0x63650>
  4020eb: 83 c0 01                     	addl	$0x1, %eax
  4020ee: 8b f0                        	movl	%eax, %esi
  4020f0: 83 c4 08                     	addl	$0x8, %esp
  4020f3: 80 3e 00                     	cmpb	$0x0, (%esi)
  4020f6: 75 ac                        	jne	0x4020a4 <.text+0x10a4>
  4020f8: 32 c0                        	xorb	%al, %al
  4020fa: 5d                           	popl	%ebp
  4020fb: 8b 8c 24 6c 02 00 00         	movl	0x26c(%esp), %ecx
  402102: 5f                           	popl	%edi
  402103: 5e                           	popl	%esi
  402104: 5b                           	popl	%ebx
  402105: 33 cc                        	xorl	%esp, %ecx
  402107: e8 b0 1a 06 00               	calll	0x463bbc <.text+0x62bbc>
  40210c: 81 c4 64 02 00 00            	addl	$0x264, %esp            # imm = 0x264
  402112: c2 04 00                     	retl	$0x4
  402115: ba 4c bf 4c 00               	movl	$0x4cbf4c, %edx         # imm = 0x4CBF4C
  40211a: 8b c6                        	movl	%esi, %eax
  40211c: 2b d6                        	subl	%esi, %edx
  40211e: 8b ff                        	movl	%edi, %edi
  402120: 8a 08                        	movb	(%eax), %cl
  402122: 88 0c 02                     	movb	%cl, (%edx,%eax)
  402125: 83 c0 01                     	addl	$0x1, %eax
  402128: 84 c9                        	testb	%cl, %cl
  40212a: 75 f4                        	jne	0x402120 <.text+0x1120>
  40212c: b0 01                        	movb	$0x1, %al
  40212e: eb ca                        	jmp	0x4020fa <.text+0x10fa>
  402130: 8a c3                        	movb	%bl, %al
  402132: eb c7                        	jmp	0x4020fb <.text+0x10fb>
  402134: cc                           	int3
  402135: cc                           	int3
  402136: cc                           	int3
  402137: cc                           	int3
  402138: cc                           	int3
  402139: cc                           	int3
  40213a: cc                           	int3
  40213b: cc                           	int3
  40213c: cc                           	int3
  40213d: cc                           	int3
  40213e: cc                           	int3
  40213f: cc                           	int3
  402140: 53                           	pushl	%ebx
  402141: 56                           	pushl	%esi
  402142: 8b 74 24 0c                  	movl	0xc(%esp), %esi
  402146: 56                           	pushl	%esi
  402147: 32 db                        	xorb	%bl, %bl
  402149: ff 15 90 30 4a 00            	calll	*0x4a3090
  40214f: 83 f8 05                     	cmpl	$0x5, %eax
  402152: 75 1f                        	jne	0x402173 <.text+0x1173>
  402154: ba 4c bf 4c 00               	movl	$0x4cbf4c, %edx         # imm = 0x4CBF4C
  402159: 8b c6                        	movl	%esi, %eax
  40215b: 2b d6                        	subl	%esi, %edx
  40215d: 8d 49 00                     	leal	(%ecx), %ecx
  402160: 8a 08                        	movb	(%eax), %cl
  402162: 88 0c 02                     	movb	%cl, (%edx,%eax)
  402165: 83 c0 01                     	addl	$0x1, %eax
  402168: 84 c9                        	testb	%cl, %cl
  40216a: 75 f4                        	jne	0x402160 <.text+0x1160>
  40216c: 5e                           	popl	%esi
  40216d: b0 01                        	movb	$0x1, %al
  40216f: 5b                           	popl	%ebx
  402170: c2 04 00                     	retl	$0x4
  402173: 5e                           	popl	%esi
  402174: 8a c3                        	movb	%bl, %al
  402176: 5b                           	popl	%ebx
  402177: c2 04 00                     	retl	$0x4
  40217a: cc                           	int3
  40217b: cc                           	int3
  40217c: cc                           	int3
  40217d: cc                           	int3
  40217e: cc                           	int3
  40217f: cc                           	int3
  402180: 56                           	pushl	%esi
  402181: 8b f1                        	movl	%ecx, %esi
  402183: 8d 46 04                     	leal	0x4(%esi), %eax
  402186: 50                           	pushl	%eax
  402187: c7 06 00 00 00 00            	movl	$0x0, (%esi)
  40218d: c6 46 1c 00                  	movb	$0x0, 0x1c(%esi)
  402191: ff 15 18 32 4a 00            	calll	*0x4a3218
  402197: 8b c6                        	movl	%esi, %eax
  402199: 5e                           	popl	%esi
  40219a: c3                           	retl
  40219b: cc                           	int3
  40219c: cc                           	int3
  40219d: cc                           	int3
  40219e: cc                           	int3
  40219f: cc                           	int3
  4021a0: 56                           	pushl	%esi
  4021a1: 8b f1                        	movl	%ecx, %esi
  4021a3: e8 08 fd ff ff               	calll	0x401eb0 <.text+0xeb0>
  4021a8: 8b 06                        	movl	(%esi), %eax
  4021aa: 85 c0                        	testl	%eax, %eax
  4021ac: c6 46 1c 01                  	movb	$0x1, 0x1c(%esi)
  4021b0: 74 31                        	je	0x4021e3 <.text+0x11e3>
  4021b2: 68 e8 03 00 00               	pushl	$0x3e8                  # imm = 0x3E8
  4021b7: 50                           	pushl	%eax
  4021b8: ff 15 80 30 4a 00            	calll	*0x4a3080
  4021be: 3d 80 00 00 00               	cmpl	$0x80, %eax
  4021c3: 75 0b                        	jne	0x4021d0 <.text+0x11d0>
  4021c5: 8b 06                        	movl	(%esi), %eax
  4021c7: 6a ff                        	pushl	$-0x1
  4021c9: 50                           	pushl	%eax
  4021ca: ff 15 7c 30 4a 00            	calll	*0x4a307c
  4021d0: 8b 06                        	movl	(%esi), %eax
  4021d2: 85 c0                        	testl	%eax, %eax
  4021d4: 74 0d                        	je	0x4021e3 <.text+0x11e3>
  4021d6: 50                           	pushl	%eax
  4021d7: ff 15 08 32 4a 00            	calll	*0x4a3208
  4021dd: c7 06 00 00 00 00            	movl	$0x0, (%esi)
  4021e3: 5e                           	popl	%esi
  4021e4: c3                           	retl
  4021e5: cc                           	int3
  4021e6: cc                           	int3
  4021e7: cc                           	int3
  4021e8: cc                           	int3
  4021e9: cc                           	int3
  4021ea: cc                           	int3
  4021eb: cc                           	int3
  4021ec: cc                           	int3
  4021ed: cc                           	int3
  4021ee: cc                           	int3
  4021ef: cc                           	int3
  4021f0: c7 05 60 c0 4c 00 00 00 00 00	movl	$0x0, 0x4cc060
  4021fa: e9 71 fd ff ff               	jmp	0x401f70 <.text+0xf70>


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  437570: 80 3d b4 d4 4b 00 00         	cmpb	$0x0, 0x4bd4b4
  437577: 74 21                        	je	0x43759a <.text+0x3659a>
  437579: 83 3d 58 c3 56 00 00         	cmpl	$0x0, 0x56c358
  437580: c6 05 b4 d4 4b 00 00         	movb	$0x0, 0x4bd4b4
  437587: 75 11                        	jne	0x43759a <.text+0x3659a>
  437589: 8b 44 24 0c                  	movl	0xc(%esp), %eax
  43758d: 8b 08                        	movl	(%eax), %ecx
  43758f: 51                           	pushl	%ecx
  437590: b9 74 c7 4c 00               	movl	$0x4cc774, %ecx         # imm = 0x4CC774
  437595: e8 b6 aa fc ff               	calll	0x402050 <.text+0x1050>
  43759a: 33 c0                        	xorl	%eax, %eax
  43759c: c3                           	retl
  43759d: cc                           	int3
  43759e: cc                           	int3
  43759f: cc                           	int3
  4375a0: 8b 44 24 08                  	movl	0x8(%esp), %eax
  4375a4: 83 e8 01                     	subl	$0x1, %eax
  4375a7: 56                           	pushl	%esi
  4375a8: 74 7d                        	je	0x437627 <.text+0x36627>
  4375aa: 83 e8 01                     	subl	$0x1, %eax
  4375ad: 0f 85 ca 00 00 00            	jne	0x43767d <.text+0x3667d>
  4375b3: 8b 44 24 10                  	movl	0x10(%esp), %eax
  4375b7: 8b 48 04                     	movl	0x4(%eax), %ecx
  4375ba: 8b 00                        	movl	(%eax), %eax
  4375bc: 8b 10                        	movl	(%eax), %edx
  4375be: 8b 74 24 08                  	movl	0x8(%esp), %esi
  4375c2: 89 56 54                     	movl	%edx, 0x54(%esi)
  4375c5: 8b 01                        	movl	(%ecx), %eax
  4375c7: 83 e8 01                     	subl	$0x1, %eax
  4375ca: 74 09                        	je	0x4375d5 <.text+0x365d5>
  4375cc: c7 46 58 00 00 00 00         	movl	$0x0, 0x58(%esi)
  4375d3: eb 07                        	jmp	0x4375dc <.text+0x365dc>
  4375d5: c7 46 58 01 00 00 00         	movl	$0x1, 0x58(%esi)
  4375dc: 85 d2                        	testl	%edx, %edx
  4375de: 56                           	pushl	%esi
  4375df: 74 2c                        	je	0x43760d <.text+0x3660d>
  4375e1: e8 0a c3 ff ff               	calll	0x4338f0 <.text+0x328f0>
  4375e6: 83 c4 04                     	addl	$0x4, %esp
  4375e9: 84 c0                        	testb	%al, %al
  4375eb: 74 0a                        	je	0x4375f7 <.text+0x365f7>
  4375ed: b9 74 c7 4c 00               	movl	$0x4cc774, %ecx         # imm = 0x4CC774
  4375f2: e8 f9 ab fc ff               	calll	0x4021f0 <.text+0x11f0>
  4375f7: 8b 46 58                     	movl	0x58(%esi), %eax
  4375fa: 8b 4e 54                     	movl	0x54(%esi), %ecx
  4375fd: 50                           	pushl	%eax
  4375fe: 51                           	pushl	%ecx
  4375ff: b9 74 c7 4c 00               	movl	$0x4cc774, %ecx         # imm = 0x4CC774
  437604: e8 f7 a8 fc ff               	calll	0x401f00 <.text+0xf00>
  437609: 33 c0                        	xorl	%eax, %eax
  43760b: 5e                           	popl	%esi
  43760c: c3                           	retl
  43760d: e8 de c2 ff ff               	calll	0x4338f0 <.text+0x328f0>
  437612: 83 c4 04                     	addl	$0x4, %esp
  437615: 84 c0                        	testb	%al, %al
  437617: b9 74 c7 4c 00               	movl	$0x4cc774, %ecx         # imm = 0x4CC774
  43761c: 74 5a                        	je	0x437678 <.text+0x36678>
  43761e: e8 cd ab fc ff               	calll	0x4021f0 <.text+0x11f0>
  437623: 33 c0                        	xorl	%eax, %eax
  437625: 5e                           	popl	%esi
  437626: c3                           	retl
  437627: 8b 54 24 10                  	movl	0x10(%esp), %edx
  43762b: 8b 02                        	movl	(%edx), %eax
  43762d: 8b 00                        	movl	(%eax), %eax
  43762f: 85 c0                        	testl	%eax, %eax
  437631: 8b 74 24 08                  	movl	0x8(%esp), %esi
  437635: 89 46 54                     	movl	%eax, 0x54(%esi)
  437638: 56                           	pushl	%esi
  437639: 74 2c                        	je	0x437667 <.text+0x36667>
  43763b: e8 b0 c2 ff ff               	calll	0x4338f0 <.text+0x328f0>
  437640: 83 c4 04                     	addl	$0x4, %esp
  437643: 84 c0                        	testb	%al, %al
  437645: 74 0a                        	je	0x437651 <.text+0x36651>
  437647: b9 74 c7 4c 00               	movl	$0x4cc774, %ecx         # imm = 0x4CC774
  43764c: e8 9f ab fc ff               	calll	0x4021f0 <.text+0x11f0>
  437651: 8b 4e 58                     	movl	0x58(%esi), %ecx
  437654: 8b 56 54                     	movl	0x54(%esi), %edx
  437657: 51                           	pushl	%ecx
  437658: 52                           	pushl	%edx
  437659: b9 74 c7 4c 00               	movl	$0x4cc774, %ecx         # imm = 0x4CC774
  43765e: e8 9d a8 fc ff               	calll	0x401f00 <.text+0xf00>
  437663: 33 c0                        	xorl	%eax, %eax
  437665: 5e                           	popl	%esi
  437666: c3                           	retl
  437667: e8 84 c2 ff ff               	calll	0x4338f0 <.text+0x328f0>
  43766c: 83 c4 04                     	addl	$0x4, %esp
  43766f: 84 c0                        	testb	%al, %al
  437671: b9 74 c7 4c 00               	movl	$0x4cc774, %ecx         # imm = 0x4CC774
  437676: 75 a6                        	jne	0x43761e <.text+0x3661e>
  437678: e8 f3 a8 fc ff               	calll	0x401f70 <.text+0xf70>
  43767d: 33 c0                        	xorl	%eax, %eax
  43767f: 5e                           	popl	%esi
  437680: c3                           	retl
  437681: cc                           	int3
  437682: cc                           	int3
  437683: cc                           	int3
  437684: cc                           	int3
  437685: cc                           	int3
  437686: cc                           	int3
  437687: cc                           	int3
  437688: cc                           	int3
  437689: cc                           	int3
  43768a: cc                           	int3
  43768b: cc                           	int3
  43768c: cc                           	int3
  43768d: cc                           	int3
  43768e: cc                           	int3
  43768f: cc                           	int3
  437690: 8b 44 24 0c                  	movl	0xc(%esp), %eax
  437694: 8b 08                        	movl	(%eax), %ecx
  437696: 8b 01                        	movl	(%ecx), %eax
  437698: 83 e8 01                     	subl	$0x1, %eax
  43769b: 8b 44 24 04                  	movl	0x4(%esp), %eax
  43769f: 74 19                        	je	0x4376ba <.text+0x366ba>
  4376a1: 83 c0 58                     	addl	$0x58, %eax
  4376a4: c7 00 00 00 00 00            	movl	$0x0, (%eax)
  4376aa: 8b 10                        	movl	(%eax), %edx
  4376ac: 52                           	pushl	%edx
  4376ad: b9 74 c7 4c 00               	movl	$0x4cc774, %ecx         # imm = 0x4CC774
  4376b2: e8 f9 a8 fc ff               	calll	0x401fb0 <.text+0xfb0>
  4376b7: 33 c0                        	xorl	%eax, %eax
  4376b9: c3                           	retl
  4376ba: 83 c0 58                     	addl	$0x58, %eax
  4376bd: c7 00 01 00 00 00            	movl	$0x1, (%eax)
  4376c3: 8b 10                        	movl	(%eax), %edx
  4376c5: 52                           	pushl	%edx
  4376c6: b9 74 c7 4c 00               	movl	$0x4cc774, %ecx         # imm = 0x4CC774
  4376cb: e8 e0 a8 fc ff               	calll	0x401fb0 <.text+0xfb0>
  4376d0: 33 c0                        	xorl	%eax, %eax
  4376d2: c3                           	retl
  4376d3: cc                           	int3
  4376d4: cc                           	int3
  4376d5: cc                           	int3
  4376d6: cc                           	int3
  4376d7: cc                           	int3
  4376d8: cc                           	int3
  4376d9: cc                           	int3
  4376da: cc                           	int3
  4376db: cc                           	int3
  4376dc: cc                           	int3
  4376dd: cc                           	int3
  4376de: cc                           	int3
  4376df: cc                           	int3
  4376e0: 33 c0                        	xorl	%eax, %eax
  4376e2: c3                           	retl


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  43a380: 83 7c 24 08 01               	cmpl	$0x1, 0x8(%esp)
  43a385: 7c 21                        	jl	0x43a3a8 <.text+0x393a8>
  43a387: 8b 44 24 0c                  	movl	0xc(%esp), %eax
  43a38b: 8b 08                        	movl	(%eax), %ecx
  43a38d: 83 39 00                     	cmpl	$0x0, (%ecx)
  43a390: b9 74 c7 4c 00               	movl	$0x4cc774, %ecx         # imm = 0x4CC774
  43a395: 74 0a                        	je	0x43a3a1 <.text+0x393a1>
  43a397: 6a 01                        	pushl	$0x1
  43a399: e8 62 7c fc ff               	calll	0x402000 <.text+0x1000>
  43a39e: 33 c0                        	xorl	%eax, %eax
  43a3a0: c3                           	retl
  43a3a1: 6a 00                        	pushl	$0x0
  43a3a3: e8 58 7c fc ff               	calll	0x402000 <.text+0x1000>
  43a3a8: 33 c0                        	xorl	%eax, %eax
  43a3aa: c3                           	retl


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  43a690: 6a 0c                        	pushl	$0xc
  43a692: e8 da 93 02 00               	calll	0x463a71 <.text+0x62a71>
  43a697: 33 c9                        	xorl	%ecx, %ecx
  43a699: 83 c4 04                     	addl	$0x4, %esp
  43a69c: 3b c1                        	cmpl	%ecx, %eax
  43a69e: 74 0a                        	je	0x43a6aa <.text+0x396aa>
  43a6a0: 89 08                        	movl	%ecx, (%eax)
  43a6a2: 89 48 04                     	movl	%ecx, 0x4(%eax)
  43a6a5: 89 48 08                     	movl	%ecx, 0x8(%eax)
  43a6a8: eb 02                        	jmp	0x43a6ac <.text+0x396ac>
  43a6aa: 33 c0                        	xorl	%eax, %eax
  43a6ac: 33 c9                        	xorl	%ecx, %ecx
  43a6ae: c7 00 01 00 00 00            	movl	$0x1, (%eax)
  43a6b4: 80 3d a4 c7 4c 00 01         	cmpb	$0x1, 0x4cc7a4
  43a6bb: 0f 94 c1                     	sete	%cl
  43a6be: 89 48 04                     	movl	%ecx, 0x4(%eax)
  43a6c1: c3                           	retl
  43a6c2: cc                           	int3
  43a6c3: cc                           	int3
  43a6c4: cc                           	int3
  43a6c5: cc                           	int3
  43a6c6: cc                           	int3
  43a6c7: cc                           	int3
  43a6c8: cc                           	int3
  43a6c9: cc                           	int3
  43a6ca: cc                           	int3
  43a6cb: cc                           	int3
  43a6cc: cc                           	int3
  43a6cd: cc                           	int3
  43a6ce: cc                           	int3
  43a6cf: cc                           	int3
  43a6d0: 53                           	pushl	%ebx
  43a6d1: 8b 5c 24 08                  	movl	0x8(%esp), %ebx
  43a6d5: 56                           	pushl	%esi
  43a6d6: 8b 74 24 10                  	movl	0x10(%esp), %esi
  43a6da: 85 f6                        	testl	%esi, %esi
  43a6dc: 7e 3b                        	jle	0x43a719 <.text+0x39719>
  43a6de: 57                           	pushl	%edi
  43a6df: 68 80 00 00 00               	pushl	$0x80


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  43e830: 6a ff                        	pushl	$-0x1
  43e832: 68 8b 1e 4a 00               	pushl	$0x4a1e8b               # imm = 0x4A1E8B
  43e837: 64 a1 00 00 00 00            	movl	%fs:0x0, %eax
  43e83d: 50                           	pushl	%eax
  43e83e: 81 ec 64 02 00 00            	subl	$0x264, %esp            # imm = 0x264
  43e844: 56                           	pushl	%esi
  43e845: a1 48 ea 4b 00               	movl	0x4bea48, %eax
  43e84a: 33 c4                        	xorl	%esp, %eax
  43e84c: 50                           	pushl	%eax
  43e84d: 8d 84 24 6c 02 00 00         	leal	0x26c(%esp), %eax
  43e854: 64 a3 00 00 00 00            	movl	%eax, %fs:0x0
  43e85a: 8b 84 24 84 02 00 00         	movl	0x284(%esp), %eax
  43e861: 8b 30                        	movl	(%eax), %esi
  43e863: 83 3e 00                     	cmpl	$0x0, (%esi)
  43e866: 7e 7e                        	jle	0x43e8e6 <.text+0x3d8e6>
  43e868: 6a 02                        	pushl	$0x2
  43e86a: b9 74 c7 4c 00               	movl	$0x4cc774, %ecx         # imm = 0x4CC774
  43e86f: e8 4c 37 fc ff               	calll	0x401fc0 <.text+0xfc0>
  43e874: 8b 0e                        	movl	(%esi), %ecx
  43e876: 3b c1                        	cmpl	%ecx, %eax
  43e878: 73 6c                        	jae	0x43e8e6 <.text+0x3d8e6>
  43e87a: 2b c8                        	subl	%eax, %ecx
  43e87c: 8b f1                        	movl	%ecx, %esi
  43e87e: 68 a0 70 55 00               	pushl	$0x5570a0               # imm = 0x5570A0
  43e883: 8d 4c 24 0c                  	leal	0xc(%esp), %ecx
  43e887: e8 64 c4 fd ff               	calll	0x41acf0 <.text+0x19cf0>
  43e88c: 6a 00                        	pushl	$0x0
  43e88e: 8d 4c 24 0c                  	leal	0xc(%esp), %ecx
  43e892: c7 84 24 78 02 00 00 00 00 00 00     	movl	$0x0, 0x278(%esp)
  43e89d: e8 7e 9c fd ff               	calll	0x418520 <.text+0x17520>
  43e8a2: 8b 0d 28 71 55 00            	movl	0x557128, %ecx
  43e8a8: 8b 15 2c 71 55 00            	movl	0x55712c, %edx
  43e8ae: 51                           	pushl	%ecx
  43e8af: 52                           	pushl	%edx
  43e8b0: 6a 00                        	pushl	$0x0
  43e8b2: 6a 00                        	pushl	$0x0
  43e8b4: 8b c8                        	movl	%eax, %ecx
  43e8b6: e8 95 9a fd ff               	calll	0x418350 <.text+0x17350>
  43e8bb: 56                           	pushl	%esi
  43e8bc: 8d 4c 24 0c                  	leal	0xc(%esp), %ecx
  43e8c0: c6 44 24 31 01               	movb	$0x1, 0x31(%esp)
  43e8c5: e8 b6 b2 fd ff               	calll	0x419b80 <.text+0x18b80>
  43e8ca: 8d 4c 24 08                  	leal	0x8(%esp), %ecx
  43e8ce: c7 84 24 74 02 00 00 ff ff ff ff     	movl	$0xffffffff, 0x274(%esp) # imm = 0xFFFFFFFF
  43e8d9: c7 44 24 08 74 39 4a 00      	movl	$0x4a3974, 0x8(%esp)    # imm = 0x4A3974
  43e8e1: e8 4a b2 fd ff               	calll	0x419b30 <.text+0x18b30>
  43e8e6: 33 c0                        	xorl	%eax, %eax
  43e8e8: 8b 8c 24 6c 02 00 00         	movl	0x26c(%esp), %ecx
  43e8ef: 64 89 0d 00 00 00 00         	movl	%ecx, %fs:0x0


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  42ba49: 68 04 01 00 00               	pushl	$0x104                  # imm = 0x104
  42ba4e: 53                           	pushl	%ebx
  42ba4f: 8d 95 90 3c 01 00            	leal	0x13c90(%ebp), %edx
  42ba55: 89 85 88 3c 01 00            	movl	%eax, 0x13c88(%ebp)
  42ba5b: 89 9d 8c 3c 01 00            	movl	%ebx, 0x13c8c(%ebp)
  42ba61: 89 9d a0 40 01 00            	movl	%ebx, 0x140a0(%ebp)
  42ba67: 89 9d a4 40 01 00            	movl	%ebx, 0x140a4(%ebp)
  42ba6d: 88 9d a8 40 01 00            	movb	%bl, 0x140a8(%ebp)
  42ba73: c6 85 a9 40 01 00 01         	movb	$0x1, 0x140a9(%ebp)
  42ba7a: 88 9d aa 40 01 00            	movb	%bl, 0x140aa(%ebp)
  42ba80: 89 9d ac 40 01 00            	movl	%ebx, 0x140ac(%ebp)
  42ba86: 88 9d b0 40 01 00            	movb	%bl, 0x140b0(%ebp)
  42ba8c: 88 9d b1 40 01 00            	movb	%bl, 0x140b1(%ebp)
  42ba92: c7 85 b4 40 01 00 1e 00 00 00	movl	$0x1e, 0x140b4(%ebp)
  42ba9c: 89 9d b8 40 01 00            	movl	%ebx, 0x140b8(%ebp)
  42baa2: c7 85 bc 40 01 00 09 00 00 00	movl	$0x9, 0x140bc(%ebp)
  42baac: 89 9d c0 40 01 00            	movl	%ebx, 0x140c0(%ebp)
  42bab2: 89 9d c4 40 01 00            	movl	%ebx, 0x140c4(%ebp)
  42bab8: 88 9d c8 40 01 00            	movb	%bl, 0x140c8(%ebp)
  42babe: 88 9d c9 40 01 00            	movb	%bl, 0x140c9(%ebp)
  42bac4: 88 9d ca 40 01 00            	movb	%bl, 0x140ca(%ebp)
  42baca: 88 9d cb 40 01 00            	movb	%bl, 0x140cb(%ebp)
  42bad0: 88 9d cc 40 01 00            	movb	%bl, 0x140cc(%ebp)
  42bad6: 88 9d cd 40 01 00            	movb	%bl, 0x140cd(%ebp)
  42badc: 52                           	pushl	%edx
  42badd: e8 2e 88 03 00               	calll	0x464310 <.text+0x63310>
  42bae2: 68 04 01 00 00               	pushl	$0x104                  # imm = 0x104
  42bae7: 8d 85 94 3d 01 00            	leal	0x13d94(%ebp), %eax
  42baed: 53                           	pushl	%ebx
  42baee: 50                           	pushl	%eax
  42baef: e8 1c 88 03 00               	calll	0x464310 <.text+0x63310>
  42baf4: 68 04 01 00 00               	pushl	$0x104                  # imm = 0x104
  42baf9: 8d 8d 98 3e 01 00            	leal	0x13e98(%ebp), %ecx
  42baff: 53                           	pushl	%ebx
  42bb00: 51                           	pushl	%ecx
  42bb01: e8 0a 88 03 00               	calll	0x464310 <.text+0x63310>
  42bb06: 68 04 01 00 00               	pushl	$0x104                  # imm = 0x104
  42bb0b: 8d 95 9c 3f 01 00            	leal	0x13f9c(%ebp), %edx
  42bb11: 53                           	pushl	%ebx
  42bb12: 52                           	pushl	%edx
  42bb13: e8 f8 87 03 00               	calll	0x464310 <.text+0x63310>
  42bb18: 89 b5 d0 40 01 00            	movl	%esi, 0x140d0(%ebp)
  42bb1e: 89 b5 d4 40 01 00            	movl	%esi, 0x140d4(%ebp)
  42bb24: 88 9d d8 40 01 00            	movb	%bl, 0x140d8(%ebp)
  42bb2a: 88 9d d9 40 01 00            	movb	%bl, 0x140d9(%ebp)


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  42bd66: 8b 95 78 3c 01 00            	movl	0x13c78(%ebp), %edx
  42bd6c: 83 c4 04                     	addl	$0x4, %esp
  42bd6f: 89 95 64 41 01 00            	movl	%edx, 0x14164(%ebp)
  42bd75: 8b 95 7c 3c 01 00            	movl	0x13c7c(%ebp), %edx
  42bd7b: 89 9d 40 41 01 00            	movl	%ebx, 0x14140(%ebp)
  42bd81: 89 9d 44 41 01 00            	movl	%ebx, 0x14144(%ebp)
  42bd87: b8 01 00 00 00               	movl	$0x1, %eax
  42bd8c: 88 9d 50 41 01 00            	movb	%bl, 0x14150(%ebp)
  42bd92: 88 9d 52 41 01 00            	movb	%bl, 0x14152(%ebp)
  42bd98: 89 9d 54 41 01 00            	movl	%ebx, 0x14154(%ebp)
  42bd9e: 89 9d 58 41 01 00            	movl	%ebx, 0x14158(%ebp)
  42bda4: 89 9d 5c 41 01 00            	movl	%ebx, 0x1415c(%ebp)
  42bdaa: 89 9d 60 41 01 00            	movl	%ebx, 0x14160(%ebp)
  42bdb0: 89 9d 6c 41 01 00            	movl	%ebx, 0x1416c(%ebp)
  42bdb6: 89 9d 70 41 01 00            	movl	%ebx, 0x14170(%ebp)
  42bdbc: 88 9d 74 41 01 00            	movb	%bl, 0x14174(%ebp)
  42bdc2: 88 9d a8 41 01 00            	movb	%bl, 0x141a8(%ebp)
  42bdc8: 88 9d a9 41 01 00            	movb	%bl, 0x141a9(%ebp)
  42bdce: 88 9d 80 41 01 00            	movb	%bl, 0x14180(%ebp)
  42bdd4: 89 9d 84 41 01 00            	movl	%ebx, 0x14184(%ebp)
  42bdda: 89 9d 88 41 01 00            	movl	%ebx, 0x14188(%ebp)
  42bde0: 88 9d 50 42 01 00            	movb	%bl, 0x14250(%ebp)
  42bde6: 88 9d 51 42 01 00            	movb	%bl, 0x14251(%ebp)
  42bdec: 89 85 48 41 01 00            	movl	%eax, 0x14148(%ebp)
  42bdf2: 83 c9 ff                     	orl	$-0x1, %ecx
  42bdf5: 89 8d 4c 41 01 00            	movl	%ecx, 0x1414c(%ebp)
  42bdfb: 88 85 51 41 01 00            	movb	%al, 0x14151(%ebp)
  42be01: 89 95 68 41 01 00            	movl	%edx, 0x14168(%ebp)


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  42c7e0: 6a ff                        	pushl	$-0x1
  42c7e2: 68 56 1c 4a 00               	pushl	$0x4a1c56               # imm = 0x4A1C56
  42c7e7: 64 a1 00 00 00 00            	movl	%fs:0x0, %eax
  42c7ed: 50                           	pushl	%eax
  42c7ee: 81 ec f4 0e 00 00            	subl	$0xef4, %esp            # imm = 0xEF4
  42c7f4: a1 48 ea 4b 00               	movl	0x4bea48, %eax
  42c7f9: 33 c4                        	xorl	%esp, %eax
  42c7fb: 89 84 24 f0 0e 00 00         	movl	%eax, 0xef0(%esp)
  42c802: 53                           	pushl	%ebx
  42c803: 55                           	pushl	%ebp
  42c804: 56                           	pushl	%esi
  42c805: 57                           	pushl	%edi
  42c806: a1 48 ea 4b 00               	movl	0x4bea48, %eax
  42c80b: 33 c4                        	xorl	%esp, %eax
  42c80d: 50                           	pushl	%eax
  42c80e: 8d 84 24 08 0f 00 00         	leal	0xf08(%esp), %eax
  42c815: 64 a3 00 00 00 00            	movl	%eax, %fs:0x0
  42c81b: 8d 44 24 28                  	leal	0x28(%esp), %eax
  42c81f: 89 4c 24 14                  	movl	%ecx, 0x14(%esp)
  42c823: 50                           	pushl	%eax
  42c824: 8d 4c 24 28                  	leal	0x28(%esp), %ecx
  42c828: 51                           	pushl	%ecx
  42c829: 33 ff                        	xorl	%edi, %edi
  42c82b: 68 b4 44 4a 00               	pushl	$0x4a44b4               # imm = 0x4A44B4
  42c830: c6 44 24 2b 00               	movb	$0x0, 0x2b(%esp)
  42c835: 89 7c 24 30                  	movl	%edi, 0x30(%esp)
  42c839: 89 7c 24 34                  	movl	%edi, 0x34(%esp)
  42c83d: e8 ae da ff ff               	calll	0x42a2f0 <.text+0x292f0>
  42c842: 83 c4 0c                     	addl	$0xc, %esp
  42c845: 84 c0                        	testb	%al, %al
  42c847: 75 64                        	jne	0x42c8ad <.text+0x2b8ad>
  42c849: 57                           	pushl	%edi
  42c84a: 68 80 00 00 00               	pushl	$0x80
  42c84f: 6a 03                        	pushl	$0x3
  42c851: 57                           	pushl	%edi
  42c852: 6a 01                        	pushl	$0x1
  42c854: 68 00 00 00 80               	pushl	$0x80000000             # imm = 0x80000000
  42c859: 68 b4 44 4a 00               	pushl	$0x4a44b4               # imm = 0x4A44B4
  42c85e: ff 15 e8 31 4a 00            	calll	*0x4a31e8
  42c864: 8b f0                        	movl	%eax, %esi
  42c866: 83 fe ff                     	cmpl	$-0x1, %esi
  42c869: 74 42                        	je	0x42c8ad <.text+0x2b8ad>
  42c86b: 57                           	pushl	%edi
  42c86c: 56                           	pushl	%esi
  42c86d: ff 15 78 30 4a 00            	calll	*0x4a3078
  42c873: 89 44 24 28                  	movl	%eax, 0x28(%esp)
  42c877: 83 c0 01                     	addl	$0x1, %eax
  42c87a: 50                           	pushl	%eax
  42c87b: e8 81 7a 03 00               	calll	0x464301 <.text+0x63301>
  42c880: 8b 4c 24 2c                  	movl	0x2c(%esp), %ecx
  42c884: 83 c4 04                     	addl	$0x4, %esp
  42c887: 57                           	pushl	%edi
  42c888: 8d 54 24 1c                  	leal	0x1c(%esp), %edx
  42c88c: 52                           	pushl	%edx
  42c88d: 51                           	pushl	%ecx
  42c88e: 50                           	pushl	%eax
  42c88f: 56                           	pushl	%esi
  42c890: 89 44 24 38                  	movl	%eax, 0x38(%esp)
  42c894: ff 15 30 31 4a 00            	calll	*0x4a3130
  42c89a: 8b 54 24 24                  	movl	0x24(%esp), %edx
  42c89e: 8b 44 24 28                  	movl	0x28(%esp), %eax
  42c8a2: 56                           	pushl	%esi
  42c8a3: c6 04 10 00                  	movb	$0x0, (%eax,%edx)
  42c8a7: ff 15 08 32 4a 00            	calll	*0x4a3208
  42c8ad: 8b 74 24 24                  	movl	0x24(%esp), %esi
  42c8b1: 3b f7                        	cmpl	%edi, %esi
  42c8b3: 0f 84 af 11 00 00            	je	0x42da68 <.text+0x2ca68>
  42c8b9: 68 00 04 00 00               	pushl	$0x400                  # imm = 0x400
  42c8be: 8d 8c 24 08 07 00 00         	leal	0x708(%esp), %ecx
  42c8c5: 57                           	pushl	%edi
  42c8c6: 51                           	pushl	%ecx
  42c8c7: c6 44 24 2b 01               	movb	$0x1, 0x2b(%esp)
  42c8cc: e8 3f 7a 03 00               	calll	0x464310 <.text+0x63310>
  42c8d1: 83 c4 0c                     	addl	$0xc, %esp
  42c8d4: 80 3e 00                     	cmpb	$0x0, (%esi)
  42c8d7: 89 74 24 20                  	movl	%esi, 0x20(%esp)
  42c8db: 0f 84 7a 11 00 00            	je	0x42da5b <.text+0x2ca5b>
  42c8e1: 68 00 04 00 00               	pushl	$0x400                  # imm = 0x400
  42c8e6: 8d 94 24 08 03 00 00         	leal	0x308(%esp), %edx
  42c8ed: 6a 00                        	pushl	$0x0
  42c8ef: 52                           	pushl	%edx
  42c8f0: e8 1b 7a 03 00               	calll	0x464310 <.text+0x63310>
  42c8f5: 8b 74 24 2c                  	movl	0x2c(%esp), %esi
  42c8f9: 0f b6 06                     	movzbl	(%esi), %eax
  42c8fc: 83 c4 0c                     	addl	$0xc, %esp
  42c8ff: 3c 0d                        	cmpb	$0xd, %al
  42c901: 8d bc 24 04 03 00 00         	leal	0x304(%esp), %edi
  42c908: 0f 84 35 11 00 00            	je	0x42da43 <.text+0x2ca43>
  42c90e: 3c 0a                        	cmpb	$0xa, %al
  42c910: 0f 84 2d 11 00 00            	je	0x42da43 <.text+0x2ca43>
  42c916: 84 c0                        	testb	%al, %al
  42c918: 8b 2d a0 30 4a 00            	movl	0x4a30a0, %ebp
  42c91e: 74 40                        	je	0x42c960 <.text+0x2b960>
  42c920: 50                           	pushl	%eax
  42c921: 6a 00                        	pushl	$0x0
  42c923: ff d5                        	calll	*%ebp
  42c925: 83 f8 01                     	cmpl	$0x1, %eax
  42c928: 75 11                        	jne	0x42c93b <.text+0x2b93b>
  42c92a: 8a 06                        	movb	(%esi), %al
  42c92c: 88 07                        	movb	%al, (%edi)
  42c92e: 8a 4e 01                     	movb	0x1(%esi), %cl
  42c931: 83 c7 01                     	addl	$0x1, %edi
  42c934: 83 c6 01                     	addl	$0x1, %esi
  42c937: 88 0f                        	movb	%cl, (%edi)
  42c939: eb 04                        	jmp	0x42c93f <.text+0x2b93f>
  42c93b: 8a 16                        	movb	(%esi), %dl
  42c93d: 88 17                        	movb	%dl, (%edi)
  42c93f: 8a 46 01                     	movb	0x1(%esi), %al
  42c942: 83 c6 01                     	addl	$0x1, %esi
  42c945: 83 c7 01                     	addl	$0x1, %edi
  42c948: 3c 0d                        	cmpb	$0xd, %al
  42c94a: 74 0a                        	je	0x42c956 <.text+0x2b956>
  42c94c: 3c 0a                        	cmpb	$0xa, %al
  42c94e: 74 06                        	je	0x42c956 <.text+0x2b956>
  42c950: 84 c0                        	testb	%al, %al
  42c952: 75 cc                        	jne	0x42c920 <.text+0x2b920>
  42c954: eb 06                        	jmp	0x42c95c <.text+0x2b95c>
  42c956: c6 07 00                     	movb	$0x0, (%edi)
  42c959: 83 c6 01                     	addl	$0x1, %esi
  42c95c: 89 74 24 20                  	movl	%esi, 0x20(%esp)
  42c960: 8d 84 24 04 03 00 00         	leal	0x304(%esp), %eax
  42c967: c6 07 00                     	movb	$0x0, (%edi)
  42c96a: 8d 50 01                     	leal	0x1(%eax), %edx
  42c96d: 8d 49 00                     	leal	(%ecx), %ecx
  42c970: 8a 08                        	movb	(%eax), %cl
  42c972: 83 c0 01                     	addl	$0x1, %eax
  42c975: 84 c9                        	testb	%cl, %cl
  42c977: 75 f7                        	jne	0x42c970 <.text+0x2b970>
  42c979: 2b c2                        	subl	%edx, %eax
  42c97b: 38 8c 24 04 03 00 00         	cmpb	%cl, 0x304(%esp)
  42c982: 8d 78 01                     	leal	0x1(%eax), %edi
  42c985: 89 7c 24 18                  	movl	%edi, 0x18(%esp)
  42c989: 8d b4 24 04 03 00 00         	leal	0x304(%esp), %esi
  42c990: 74 49                        	je	0x42c9db <.text+0x2b9db>
  42c992: 8a 06                        	movb	(%esi), %al
  42c994: 3c 20                        	cmpb	$0x20, %al
  42c996: 74 20                        	je	0x42c9b8 <.text+0x2b9b8>
  42c998: 3c 2c                        	cmpb	$0x2c, %al
  42c99a: 74 1c                        	je	0x42c9b8 <.text+0x2b9b8>
  42c99c: 3c 09                        	cmpb	$0x9, %al
  42c99e: 74 18                        	je	0x42c9b8 <.text+0x2b9b8>
  42c9a0: 3c 2f                        	cmpb	$0x2f, %al
  42c9a2: 75 05                        	jne	0x42c9a9 <.text+0x2b9a9>
  42c9a4: 38 46 01                     	cmpb	%al, 0x1(%esi)
  42c9a7: 74 1c                        	je	0x42c9c5 <.text+0x2b9c5>
  42c9a9: 50                           	pushl	%eax
  42c9aa: 6a 00                        	pushl	$0x0
  42c9ac: ff d5                        	calll	*%ebp
  42c9ae: 83 f8 01                     	cmpl	$0x1, %eax
  42c9b1: 75 08                        	jne	0x42c9bb <.text+0x2b9bb>
  42c9b3: 83 c6 02                     	addl	$0x2, %esi
  42c9b6: eb 06                        	jmp	0x42c9be <.text+0x2b9be>
  42c9b8: c6 06 00                     	movb	$0x0, (%esi)
  42c9bb: 83 c6 01                     	addl	$0x1, %esi
  42c9be: 80 3e 00                     	cmpb	$0x0, (%esi)
  42c9c1: 75 cf                        	jne	0x42c992 <.text+0x2b992>
  42c9c3: eb 16                        	jmp	0x42c9db <.text+0x2b9db>
  42c9c5: 80 3e 00                     	cmpb	$0x0, (%esi)
  42c9c8: 74 11                        	je	0x42c9db <.text+0x2b9db>
  42c9ca: 8d 9b 00 00 00 00            	leal	(%ebx), %ebx
  42c9d0: c6 06 00                     	movb	$0x0, (%esi)
  42c9d3: 83 c6 01                     	addl	$0x1, %esi
  42c9d6: 80 3e 00                     	cmpb	$0x0, (%esi)
  42c9d9: 75 f5                        	jne	0x42c9d0 <.text+0x2b9d0>
  42c9db: 33 db                        	xorl	%ebx, %ebx
  42c9dd: 33 f6                        	xorl	%esi, %esi
  42c9df: 85 ff                        	testl	%edi, %edi
  42c9e1: 7e 7a                        	jle	0x42ca5d <.text+0x2ba5d>
  42c9e3: 8a 8c 34 04 03 00 00         	movb	0x304(%esp,%esi), %cl
  42c9ea: 80 f9 3d                     	cmpb	$0x3d, %cl
  42c9ed: 8d 84 34 04 03 00 00         	leal	0x304(%esp,%esi), %eax
  42c9f4: 74 59                        	je	0x42ca4f <.text+0x2ba4f>
  42c9f6: 84 c9                        	testb	%cl, %cl
  42c9f8: 74 36                        	je	0x42ca30 <.text+0x2ba30>
  42c9fa: 85 db                        	testl	%ebx, %ebx
  42c9fc: 75 5d                        	jne	0x42ca5b <.text+0x2ba5b>
  42c9fe: 84 c9                        	testb	%cl, %cl
  42ca00: 8b d8                        	movl	%eax, %ebx
  42ca02: 74 2c                        	je	0x42ca30 <.text+0x2ba30>
  42ca04: 0f b6 00                     	movzbl	(%eax), %eax
  42ca07: 3c 3d                        	cmpb	$0x3d, %al
  42ca09: 74 22                        	je	0x42ca2d <.text+0x2ba2d>
  42ca0b: 50                           	pushl	%eax
  42ca0c: 6a 00                        	pushl	$0x0
  42ca0e: ff d5                        	calll	*%ebp
  42ca10: 83 f8 01                     	cmpl	$0x1, %eax
  42ca13: 75 02                        	jne	0x42ca17 <.text+0x2ba17>
  42ca15: 03 f0                        	addl	%eax, %esi
  42ca17: 83 c6 01                     	addl	$0x1, %esi
  42ca1a: 80 bc 34 04 03 00 00 00      	cmpb	$0x0, 0x304(%esp,%esi)
  42ca22: 8d 84 34 04 03 00 00         	leal	0x304(%esp,%esi), %eax
  42ca29: 75 d9                        	jne	0x42ca04 <.text+0x2ba04>
  42ca2b: eb 03                        	jmp	0x42ca30 <.text+0x2ba30>
  42ca2d: 83 ee 01                     	subl	$0x1, %esi
  42ca30: 0f b6 84 34 04 03 00 00      	movzbl	0x304(%esp,%esi), %eax
  42ca38: 50                           	pushl	%eax
  42ca39: 6a 00                        	pushl	$0x0
  42ca3b: ff d5                        	calll	*%ebp
  42ca3d: 83 f8 01                     	cmpl	$0x1, %eax
  42ca40: 75 02                        	jne	0x42ca44 <.text+0x2ba44>
  42ca42: 03 f0                        	addl	%eax, %esi
  42ca44: 83 c6 01                     	addl	$0x1, %esi
  42ca47: 3b 74 24 18                  	cmpl	0x18(%esp), %esi
  42ca4b: 7c 96                        	jl	0x42c9e3 <.text+0x2b9e3>
  42ca4d: eb 0e                        	jmp	0x42ca5d <.text+0x2ba5d>
  42ca4f: c6 84 34 04 03 00 00 00      	movb	$0x0, 0x304(%esp,%esi)
  42ca57: 8b fe                        	movl	%esi, %edi
  42ca59: eb 02                        	jmp	0x42ca5d <.text+0x2ba5d>
  42ca5b: 33 db                        	xorl	%ebx, %ebx
  42ca5d: 6a 40                        	pushl	$0x40
  42ca5f: 33 ed                        	xorl	%ebp, %ebp
  42ca61: 8d 4c 24 30                  	leal	0x30(%esp), %ecx
  42ca65: 55                           	pushl	%ebp
  42ca66: 51                           	pushl	%ecx
  42ca67: e8 a4 78 03 00               	calll	0x464310 <.text+0x63310>
  42ca6c: 8b 54 24 24                  	movl	0x24(%esp), %edx
  42ca70: 83 c4 0c                     	addl	$0xc, %esp
  42ca73: 3b fa                        	cmpl	%edx, %edi
  42ca75: 8b c7                        	movl	%edi, %eax
  42ca77: 7d 40                        	jge	0x42cab9 <.text+0x2bab9>
  42ca79: 8d a4 24 00 00 00 00         	leal	(%esp), %esp
  42ca80: 80 bc 04 04 03 00 00 00      	cmpb	$0x0, 0x304(%esp,%eax)
  42ca88: 8d 8c 04 04 03 00 00         	leal	0x304(%esp,%eax), %ecx
  42ca8f: 74 1c                        	je	0x42caad <.text+0x2baad>
  42ca91: 89 4c ac 2c                  	movl	%ecx, 0x2c(%esp,%ebp,4)
  42ca95: 83 c5 01                     	addl	$0x1, %ebp
  42ca98: 80 39 00                     	cmpb	$0x0, (%ecx)
  42ca9b: 74 10                        	je	0x42caad <.text+0x2baad>
  42ca9d: 8d 49 00                     	leal	(%ecx), %ecx
  42caa0: 83 c0 01                     	addl	$0x1, %eax
  42caa3: 80 bc 04 04 03 00 00 00      	cmpb	$0x0, 0x304(%esp,%eax)
  42caab: 75 f3                        	jne	0x42caa0 <.text+0x2baa0>
  42caad: 83 fd 10                     	cmpl	$0x10, %ebp
  42cab0: 7f 07                        	jg	0x42cab9 <.text+0x2bab9>
  42cab2: 83 c0 01                     	addl	$0x1, %eax
  42cab5: 3b c2                        	cmpl	%edx, %eax
  42cab7: 7c c7                        	jl	0x42ca80 <.text+0x2ba80>
  42cab9: 85 db                        	testl	%ebx, %ebx
  42cabb: 0f 84 89 0f 00 00            	je	0x42da4a <.text+0x2ca4a>
  42cac1: 8b fb                        	movl	%ebx, %edi
  42cac3: be a8 44 4a 00               	movl	$0x4a44a8, %esi         # imm = 0x4A44A8
  42cac8: b9 0b 00 00 00               	movl	$0xb, %ecx
  42cacd: 33 d2                        	xorl	%edx, %edx
  42cacf: f3 a6                        	rep		cmpsb	%es:(%edi), (%esi)
  42cad1: 75 4d                        	jne	0x42cb20 <.text+0x2bb20>
  42cad3: 83 fd 02                     	cmpl	$0x2, %ebp
  42cad6: 0f 8c 6e 0f 00 00            	jl	0x42da4a <.text+0x2ca4a>
  42cadc: 8b 44 24 2c                  	movl	0x2c(%esp), %eax


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  42cb20: 8b fb                        	movl	%ebx, %edi
  42cb22: be 9c 44 4a 00               	movl	$0x4a449c, %esi         # imm = 0x4A449C
  42cb27: b9 0c 00 00 00               	movl	$0xc, %ecx
  42cb2c: 33 c0                        	xorl	%eax, %eax
  42cb2e: f3 a6                        	rep		cmpsb	%es:(%edi), (%esi)
  42cb30: 75 60                        	jne	0x42cb92 <.text+0x2bb92>
  42cb32: 83 fd 01                     	cmpl	$0x1, %ebp
  42cb35: 0f 8c 0f 0f 00 00            	jl	0x42da4a <.text+0x2ca4a>
  42cb3b: 8b 74 24 2c                  	movl	0x2c(%esp), %esi
  42cb3f: 8b 54 24 14                  	movl	0x14(%esp), %edx
  42cb43: 2b d6                        	subl	%esi, %edx
  42cb45: 8b c6                        	movl	%esi, %eax
  42cb47: 81 c2 98 3e 01 00            	addl	$0x13e98, %edx          # imm = 0x13E98
  42cb4d: 8d 49 00                     	leal	(%ecx), %ecx
  42cb50: 8a 08                        	movb	(%eax), %cl
  42cb52: 88 0c 10                     	movb	%cl, (%eax,%edx)
  42cb55: 83 c0 01                     	addl	$0x1, %eax
  42cb58: 84 c9                        	testb	%cl, %cl
  42cb5a: 75 f4                        	jne	0x42cb50 <.text+0x2bb50>
  42cb5c: 56                           	pushl	%esi
  42cb5d: e8 9e e6 ff ff               	calll	0x42b200 <.text+0x2a200>
  42cb62: 83 c4 04                     	addl	$0x4, %esp
  42cb65: 85 c0                        	testl	%eax, %eax
  42cb67: 0f 84 dd 0e 00 00            	je	0x42da4a <.text+0x2ca4a>
  42cb6d: 8b 4c 24 14                  	movl	0x14(%esp), %ecx
  42cb71: 89 81 88 3c 01 00            	movl	%eax, 0x13c88(%ecx)
  42cb77: 2b ce                        	subl	%esi, %ecx
  42cb79: 8b c6                        	movl	%esi, %eax
  42cb7b: 8d 91 90 3c 01 00            	leal	0x13c90(%ecx), %edx
  42cb81: 8a 08                        	movb	(%eax), %cl
  42cb83: 88 0c 10                     	movb	%cl, (%eax,%edx)
  42cb86: 83 c0 01                     	addl	$0x1, %eax
  42cb89: 84 c9                        	testb	%cl, %cl
  42cb8b: 75 f4                        	jne	0x42cb81 <.text+0x2bb81>
  42cb8d: e9 b8 0e 00 00               	jmp	0x42da4a <.text+0x2ca4a>
  42cb92: 8b fb                        	movl	%ebx, %edi
  42cb94: be 8c 44 4a 00               	movl	$0x4a448c, %esi         # imm = 0x4A448C
  42cb99: b9 0d 00 00 00               	movl	$0xd, %ecx
  42cb9e: 33 d2                        	xorl	%edx, %edx
  42cba0: f3 a6                        	rep		cmpsb	%es:(%edi), (%esi)
  42cba2: 75 5e                        	jne	0x42cc02 <.text+0x2bc02>
  42cba4: 83 fd 01                     	cmpl	$0x1, %ebp
  42cba7: 0f 8c 9d 0e 00 00            	jl	0x42da4a <.text+0x2ca4a>
  42cbad: 8b 74 24 2c                  	movl	0x2c(%esp), %esi
  42cbb1: 8b 54 24 14                  	movl	0x14(%esp), %edx
  42cbb5: 2b d6                        	subl	%esi, %edx
  42cbb7: 8b c6                        	movl	%esi, %eax
  42cbb9: 81 c2 9c 3f 01 00            	addl	$0x13f9c, %edx          # imm = 0x13F9C
  42cbbf: 90                           	nop
  42cbc0: 8a 08                        	movb	(%eax), %cl
  42cbc2: 88 0c 10                     	movb	%cl, (%eax,%edx)
  42cbc5: 83 c0 01                     	addl	$0x1, %eax
  42cbc8: 84 c9                        	testb	%cl, %cl
  42cbca: 75 f4                        	jne	0x42cbc0 <.text+0x2bbc0>
  42cbcc: 56                           	pushl	%esi
  42cbcd: e8 2e e6 ff ff               	calll	0x42b200 <.text+0x2a200>
  42cbd2: 83 c4 04                     	addl	$0x4, %esp
  42cbd5: 85 c0                        	testl	%eax, %eax
  42cbd7: 0f 84 6d 0e 00 00            	je	0x42da4a <.text+0x2ca4a>
  42cbdd: 8b 4c 24 14                  	movl	0x14(%esp), %ecx
  42cbe1: 89 81 8c 3c 01 00            	movl	%eax, 0x13c8c(%ecx)
  42cbe7: 2b ce                        	subl	%esi, %ecx
  42cbe9: 8b c6                        	movl	%esi, %eax
  42cbeb: 8d 91 94 3d 01 00            	leal	0x13d94(%ecx), %edx
  42cbf1: 8a 08                        	movb	(%eax), %cl
  42cbf3: 88 0c 10                     	movb	%cl, (%eax,%edx)
  42cbf6: 83 c0 01                     	addl	$0x1, %eax
  42cbf9: 84 c9                        	testb	%cl, %cl
  42cbfb: 75 f4                        	jne	0x42cbf1 <.text+0x2bbf1>
  42cbfd: e9 48 0e 00 00               	jmp	0x42da4a <.text+0x2ca4a>
  42cc02: 8b fb                        	movl	%ebx, %edi
  42cc04: be 6c 44 4a 00               	movl	$0x4a446c, %esi         # imm = 0x4A446C
  42cc09: b9 20 00 00 00               	movl	$0x20, %ecx
  42cc0e: 33 c0                        	xorl	%eax, %eax
  42cc10: f3 a6                        	rep		cmpsb	%es:(%edi), (%esi)
  42cc12: 75 27                        	jne	0x42cc3b <.text+0x2bc3b>
  42cc14: 83 fd 01                     	cmpl	$0x1, %ebp
  42cc17: 0f 8c 2d 0e 00 00            	jl	0x42da4a <.text+0x2ca4a>
  42cc1d: 8b 4c 24 2c                  	movl	0x2c(%esp), %ecx
  42cc21: 50                           	pushl	%eax
  42cc22: 50                           	pushl	%eax
  42cc23: 51                           	pushl	%ecx
  42cc24: e8 1b 94 03 00               	calll	0x466044 <.text+0x65044>
  42cc29: 8b 54 24 20                  	movl	0x20(%esp), %edx
  42cc2d: 83 c4 0c                     	addl	$0xc, %esp
  42cc30: 89 82 a4 40 01 00            	movl	%eax, 0x140a4(%edx)
  42cc36: e9 0f 0e 00 00               	jmp	0x42da4a <.text+0x2ca4a>
  42cc3b: 8b fb                        	movl	%ebx, %edi
  42cc3d: be 58 44 4a 00               	movl	$0x4a4458, %esi         # imm = 0x4A4458
  42cc42: b9 13 00 00 00               	movl	$0x13, %ecx
  42cc47: 33 c0                        	xorl	%eax, %eax
  42cc49: f3 a6                        	rep		cmpsb	%es:(%edi), (%esi)
  42cc4b: 75 27                        	jne	0x42cc74 <.text+0x2bc74>
  42cc4d: 83 fd 01                     	cmpl	$0x1, %ebp
  42cc50: 0f 8c f4 0d 00 00            	jl	0x42da4a <.text+0x2ca4a>
  42cc56: 8b 4c 24 2c                  	movl	0x2c(%esp), %ecx
  42cc5a: 50                           	pushl	%eax
  42cc5b: 50                           	pushl	%eax
  42cc5c: 51                           	pushl	%ecx
  42cc5d: e8 e2 93 03 00               	calll	0x466044 <.text+0x65044>
  42cc62: 8b 54 24 20                  	movl	0x20(%esp), %edx
  42cc66: 83 c4 0c                     	addl	$0xc, %esp
  42cc69: 89 82 a0 40 01 00            	movl	%eax, 0x140a0(%edx)
  42cc6f: e9 d6 0d 00 00               	jmp	0x42da4a <.text+0x2ca4a>


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  42d3b1: 8b fb                        	movl	%ebx, %edi
  42d3b3: be 10 42 4a 00               	movl	$0x4a4210, %esi         # imm = 0x4A4210
  42d3b8: b9 14 00 00 00               	movl	$0x14, %ecx
  42d3bd: 33 d2                        	xorl	%edx, %edx
  42d3bf: f3 a6                        	rep		cmpsb	%es:(%edi), (%esi)
  42d3c1: 75 27                        	jne	0x42d3ea <.text+0x2c3ea>
  42d3c3: 83 fd 01                     	cmpl	$0x1, %ebp
  42d3c6: 0f 8c 7e 06 00 00            	jl	0x42da4a <.text+0x2ca4a>
  42d3cc: 8b 44 24 2c                  	movl	0x2c(%esp), %eax
  42d3d0: 52                           	pushl	%edx
  42d3d1: 52                           	pushl	%edx
  42d3d2: 50                           	pushl	%eax
  42d3d3: e8 6c 8c 03 00               	calll	0x466044 <.text+0x65044>
  42d3d8: 8b 4c 24 20                  	movl	0x20(%esp), %ecx
  42d3dc: 83 c4 0c                     	addl	$0xc, %esp
  42d3df: 89 81 d4 40 01 00            	movl	%eax, 0x140d4(%ecx)
  42d3e5: e9 60 06 00 00               	jmp	0x42da4a <.text+0x2ca4a>
  42d3ea: 8b fb                        	movl	%ebx, %edi
  42d3ec: be f0 41 4a 00               	movl	$0x4a41f0, %esi         # imm = 0x4A41F0
  42d3f1: b9 1e 00 00 00               	movl	$0x1e, %ecx
  42d3f6: 33 d2                        	xorl	%edx, %edx
  42d3f8: f3 a6                        	rep		cmpsb	%es:(%edi), (%esi)
  42d3fa: 75 38                        	jne	0x42d434 <.text+0x2c434>
  42d3fc: 83 fd 01                     	cmpl	$0x1, %ebp
  42d3ff: 7c 23                        	jl	0x42d424 <.text+0x2c424>
  42d401: 8b 44 24 2c                  	movl	0x2c(%esp), %eax
  42d405: 52                           	pushl	%edx
  42d406: 52                           	pushl	%edx
  42d407: 50                           	pushl	%eax
  42d408: e8 37 8c 03 00               	calll	0x466044 <.text+0x65044>
  42d40d: 8b 54 24 20                  	movl	0x20(%esp), %edx
  42d411: 83 c4 0c                     	addl	$0xc, %esp
  42d414: 85 c0                        	testl	%eax, %eax
  42d416: 0f 95 c1                     	setne	%cl
  42d419: 88 8a d8 40 01 00            	movb	%cl, 0x140d8(%edx)
  42d41f: e9 26 06 00 00               	jmp	0x42da4a <.text+0x2ca4a>
  42d424: 8b 44 24 14                  	movl	0x14(%esp), %eax
  42d428: c6 80 d8 40 01 00 00         	movb	$0x0, 0x140d8(%eax)
  42d42f: e9 16 06 00 00               	jmp	0x42da4a <.text+0x2ca4a>
  42d434: 8b fb                        	movl	%ebx, %edi
  42d436: be d8 41 4a 00               	movl	$0x4a41d8, %esi         # imm = 0x4A41D8
  42d43b: b9 18 00 00 00               	movl	$0x18, %ecx
  42d440: 33 d2                        	xorl	%edx, %edx
  42d442: f3 a6                        	rep		cmpsb	%es:(%edi), (%esi)
  42d444: 75 38                        	jne	0x42d47e <.text+0x2c47e>
  42d446: 83 fd 01                     	cmpl	$0x1, %ebp
  42d449: 7c 23                        	jl	0x42d46e <.text+0x2c46e>
  42d44b: 8b 44 24 2c                  	movl	0x2c(%esp), %eax
  42d44f: 52                           	pushl	%edx
  42d450: 52                           	pushl	%edx
  42d451: 50                           	pushl	%eax
  42d452: e8 ed 8b 03 00               	calll	0x466044 <.text+0x65044>
  42d457: 8b 54 24 20                  	movl	0x20(%esp), %edx
  42d45b: 83 c4 0c                     	addl	$0xc, %esp
  42d45e: 85 c0                        	testl	%eax, %eax
  42d460: 0f 95 c1                     	setne	%cl
  42d463: 88 8a d9 40 01 00            	movb	%cl, 0x140d9(%edx)
  42d469: e9 dc 05 00 00               	jmp	0x42da4a <.text+0x2ca4a>
  42d46e: 8b 44 24 14                  	movl	0x14(%esp), %eax
  42d472: c6 80 d9 40 01 00 00         	movb	$0x0, 0x140d9(%eax)
  42d479: e9 cc 05 00 00               	jmp	0x42da4a <.text+0x2ca4a>
  42d47e: 8b fb                        	movl	%ebx, %edi
  42d480: be bc 41 4a 00               	movl	$0x4a41bc, %esi         # imm = 0x4A41BC
  42d485: b9 1a 00 00 00               	movl	$0x1a, %ecx
  42d48a: 33 d2                        	xorl	%edx, %edx
  42d48c: f3 a6                        	rep		cmpsb	%es:(%edi), (%esi)
  42d48e: 75 38                        	jne	0x42d4c8 <.text+0x2c4c8>
  42d490: 83 fd 01                     	cmpl	$0x1, %ebp
  42d493: 7c 23                        	jl	0x42d4b8 <.text+0x2c4b8>
  42d495: 8b 44 24 2c                  	movl	0x2c(%esp), %eax
  42d499: 52                           	pushl	%edx
  42d49a: 52                           	pushl	%edx
  42d49b: 50                           	pushl	%eax
  42d49c: e8 a3 8b 03 00               	calll	0x466044 <.text+0x65044>
  42d4a1: 8b 54 24 20                  	movl	0x20(%esp), %edx
  42d4a5: 83 c4 0c                     	addl	$0xc, %esp
  42d4a8: 85 c0                        	testl	%eax, %eax
  42d4aa: 0f 95 c1                     	setne	%cl
  42d4ad: 88 8a da 40 01 00            	movb	%cl, 0x140da(%edx)
  42d4b3: e9 92 05 00 00               	jmp	0x42da4a <.text+0x2ca4a>
  42d4b8: 8b 44 24 14                  	movl	0x14(%esp), %eax
  42d4bc: c6 80 da 40 01 00 00         	movb	$0x0, 0x140da(%eax)
  42d4c3: e9 82 05 00 00               	jmp	0x42da4a <.text+0x2ca4a>
  42d4c8: 8b fb                        	movl	%ebx, %edi
  42d4ca: be 98 41 4a 00               	movl	$0x4a4198, %esi         # imm = 0x4A4198
  42d4cf: b9 24 00 00 00               	movl	$0x24, %ecx
  42d4d4: 33 d2                        	xorl	%edx, %edx
  42d4d6: f3 a6                        	rep		cmpsb	%es:(%edi), (%esi)
  42d4d8: 0f 84 6c 05 00 00            	je	0x42da4a <.text+0x2ca4a>
  42d4de: 8b fb                        	movl	%ebx, %edi
  42d4e0: be 74 41 4a 00               	movl	$0x4a4174, %esi         # imm = 0x4A4174
  42d4e5: b9 24 00 00 00               	movl	$0x24, %ecx
  42d4ea: 33 c0                        	xorl	%eax, %eax
  42d4ec: f3 a6                        	rep		cmpsb	%es:(%edi), (%esi)
  42d4ee: 75 41                        	jne	0x42d531 <.text+0x2c531>
  42d4f0: 33 f6                        	xorl	%esi, %esi
  42d4f2: 85 ed                        	testl	%ebp, %ebp
  42d4f4: 0f 8e 50 05 00 00            	jle	0x42da4a <.text+0x2ca4a>
  42d4fa: 8b 7c 24 14                  	movl	0x14(%esp), %edi
  42d4fe: 81 c7 dc 40 01 00            	addl	$0x140dc, %edi          # imm = 0x140DC
  42d504: 8b 4c b4 2c                  	movl	0x2c(%esp,%esi,4), %ecx
  42d508: 6a 00                        	pushl	$0x0
  42d50a: 6a 00                        	pushl	$0x0
  42d50c: 51                           	pushl	%ecx
  42d50d: e8 32 8b 03 00               	calll	0x466044 <.text+0x65044>
  42d512: 83 c4 0c                     	addl	$0xc, %esp
  42d515: 8d 54 24 18                  	leal	0x18(%esp), %edx
  42d519: 52                           	pushl	%edx
  42d51a: 8b cf                        	movl	%edi, %ecx
  42d51c: 89 44 24 1c                  	movl	%eax, 0x1c(%esp)
  42d520: e8 9b ed ff ff               	calll	0x42c2c0 <.text+0x2b2c0>
  42d525: 83 c6 01                     	addl	$0x1, %esi
  42d528: 3b f5                        	cmpl	%ebp, %esi
  42d52a: 7c d8                        	jl	0x42d504 <.text+0x2c504>
  42d52c: e9 19 05 00 00               	jmp	0x42da4a <.text+0x2ca4a>
  42d531: 8b fb                        	movl	%ebx, %edi
  42d533: be 58 41 4a 00               	movl	$0x4a4158, %esi         # imm = 0x4A4158
  42d538: b9 1c 00 00 00               	movl	$0x1c, %ecx
  42d53d: 33 c0                        	xorl	%eax, %eax
  42d53f: f3 a6                        	rep		cmpsb	%es:(%edi), (%esi)
  42d541: 75 41                        	jne	0x42d584 <.text+0x2c584>
  42d543: 33 f6                        	xorl	%esi, %esi
  42d545: 85 ed                        	testl	%ebp, %ebp
  42d547: 0f 8e fd 04 00 00            	jle	0x42da4a <.text+0x2ca4a>
  42d54d: 8b 7c 24 14                  	movl	0x14(%esp), %edi
  42d551: 81 c7 ec 40 01 00            	addl	$0x140ec, %edi          # imm = 0x140EC
  42d557: 8b 4c b4 2c                  	movl	0x2c(%esp,%esi,4), %ecx
  42d55b: 6a 00                        	pushl	$0x0
  42d55d: 6a 00                        	pushl	$0x0
  42d55f: 51                           	pushl	%ecx
  42d560: e8 df 8a 03 00               	calll	0x466044 <.text+0x65044>
  42d565: 83 c4 0c                     	addl	$0xc, %esp
  42d568: 8d 54 24 18                  	leal	0x18(%esp), %edx
  42d56c: 52                           	pushl	%edx
  42d56d: 8b cf                        	movl	%edi, %ecx
  42d56f: 89 44 24 1c                  	movl	%eax, 0x1c(%esp)
  42d573: e8 48 ed ff ff               	calll	0x42c2c0 <.text+0x2b2c0>
  42d578: 83 c6 01                     	addl	$0x1, %esi
  42d57b: 3b f5                        	cmpl	%ebp, %esi
  42d57d: 7c d8                        	jl	0x42d557 <.text+0x2c557>
  42d57f: e9 c6 04 00 00               	jmp	0x42da4a <.text+0x2ca4a>
  42d584: 8b fb                        	movl	%ebx, %edi
  42d586: be 3c 41 4a 00               	movl	$0x4a413c, %esi         # imm = 0x4A413C
  42d58b: b9 1b 00 00 00               	movl	$0x1b, %ecx
  42d590: 33 c0                        	xorl	%eax, %eax
  42d592: f3 a6                        	rep		cmpsb	%es:(%edi), (%esi)
  42d594: 75 47                        	jne	0x42d5dd <.text+0x2c5dd>
  42d596: 33 f6                        	xorl	%esi, %esi
  42d598: 85 ed                        	testl	%ebp, %ebp
  42d59a: 0f 8e aa 04 00 00            	jle	0x42da4a <.text+0x2ca4a>
  42d5a0: 8b 7c 24 14                  	movl	0x14(%esp), %edi
  42d5a4: 81 c7 fc 40 01 00            	addl	$0x140fc, %edi          # imm = 0x140FC
  42d5aa: 8d 9b 00 00 00 00            	leal	(%ebx), %ebx
  42d5b0: 8b 4c b4 2c                  	movl	0x2c(%esp,%esi,4), %ecx
  42d5b4: 6a 00                        	pushl	$0x0
  42d5b6: 6a 00                        	pushl	$0x0
  42d5b8: 51                           	pushl	%ecx
  42d5b9: e8 86 8a 03 00               	calll	0x466044 <.text+0x65044>
  42d5be: 83 c4 0c                     	addl	$0xc, %esp
  42d5c1: 8d 54 24 18                  	leal	0x18(%esp), %edx
  42d5c5: 52                           	pushl	%edx
  42d5c6: 8b cf                        	movl	%edi, %ecx
  42d5c8: 89 44 24 1c                  	movl	%eax, 0x1c(%esp)
  42d5cc: e8 ef ec ff ff               	calll	0x42c2c0 <.text+0x2b2c0>
  42d5d1: 83 c6 01                     	addl	$0x1, %esi
  42d5d4: 3b f5                        	cmpl	%ebp, %esi
  42d5d6: 7c d8                        	jl	0x42d5b0 <.text+0x2c5b0>
  42d5d8: e9 6d 04 00 00               	jmp	0x42da4a <.text+0x2ca4a>
  42d5dd: 8b fb                        	movl	%ebx, %edi
  42d5df: be 1c 41 4a 00               	movl	$0x4a411c, %esi         # imm = 0x4A411C
  42d5e4: b9 1e 00 00 00               	movl	$0x1e, %ecx
  42d5e9: 33 c0                        	xorl	%eax, %eax
  42d5eb: f3 a6                        	rep		cmpsb	%es:(%edi), (%esi)
  42d5ed: 75 41                        	jne	0x42d630 <.text+0x2c630>
  42d5ef: 33 f6                        	xorl	%esi, %esi
  42d5f1: 85 ed                        	testl	%ebp, %ebp
  42d5f3: 0f 8e 51 04 00 00            	jle	0x42da4a <.text+0x2ca4a>
  42d5f9: 8b 7c 24 14                  	movl	0x14(%esp), %edi
  42d5fd: 81 c7 0c 41 01 00            	addl	$0x1410c, %edi          # imm = 0x1410C
  42d603: 8b 4c b4 2c                  	movl	0x2c(%esp,%esi,4), %ecx
  42d607: 6a 00                        	pushl	$0x0
  42d609: 6a 00                        	pushl	$0x0
  42d60b: 51                           	pushl	%ecx
  42d60c: e8 33 8a 03 00               	calll	0x466044 <.text+0x65044>
  42d611: 83 c4 0c                     	addl	$0xc, %esp
  42d614: 8d 54 24 18                  	leal	0x18(%esp), %edx
  42d618: 52                           	pushl	%edx
  42d619: 8b cf                        	movl	%edi, %ecx
  42d61b: 89 44 24 1c                  	movl	%eax, 0x1c(%esp)
  42d61f: e8 9c ec ff ff               	calll	0x42c2c0 <.text+0x2b2c0>
  42d624: 83 c6 01                     	addl	$0x1, %esi
  42d627: 3b f5                        	cmpl	%ebp, %esi
  42d629: 7c d8                        	jl	0x42d603 <.text+0x2c603>
  42d62b: e9 1a 04 00 00               	jmp	0x42da4a <.text+0x2ca4a>
  42d630: 8b fb                        	movl	%ebx, %edi
  42d632: be 0c 41 4a 00               	movl	$0x4a410c, %esi         # imm = 0x4A410C
  42d637: b9 10 00 00 00               	movl	$0x10, %ecx
  42d63c: 33 c0                        	xorl	%eax, %eax
  42d63e: f3 a6                        	rep		cmpsb	%es:(%edi), (%esi)
  42d640: 75 2f                        	jne	0x42d671 <.text+0x2c671>
  42d642: 83 fd 01                     	cmpl	$0x1, %ebp
  42d645: 0f 8c ff 03 00 00            	jl	0x42da4a <.text+0x2ca4a>
  42d64b: 8b 4c 24 2c                  	movl	0x2c(%esp), %ecx
  42d64f: 50                           	pushl	%eax
  42d650: 50                           	pushl	%eax
  42d651: 51                           	pushl	%ecx
  42d652: e8 ed 89 03 00               	calll	0x466044 <.text+0x65044>
  42d657: 83 c4 0c                     	addl	$0xc, %esp
  42d65a: 85 c0                        	testl	%eax, %eax
  42d65c: 0f 8e e8 03 00 00            	jle	0x42da4a <.text+0x2ca4a>
  42d662: 8b 54 24 14                  	movl	0x14(%esp), %edx
  42d666: 89 82 48 41 01 00            	movl	%eax, 0x14148(%edx)


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  42d726: 8b fb                        	movl	%ebx, %edi
  42d728: be e4 40 4a 00               	movl	$0x4a40e4, %esi         # imm = 0x4A40E4
  42d72d: b9 0e 00 00 00               	movl	$0xe, %ecx
  42d732: 33 d2                        	xorl	%edx, %edx
  42d734: f3 a6                        	rep		cmpsb	%es:(%edi), (%esi)
  42d736: 75 2c                        	jne	0x42d764 <.text+0x2c764>
  42d738: 83 fd 01                     	cmpl	$0x1, %ebp
  42d73b: 0f 8c 09 03 00 00            	jl	0x42da4a <.text+0x2ca4a>
  42d741: 8b 44 24 2c                  	movl	0x2c(%esp), %eax
  42d745: 52                           	pushl	%edx
  42d746: 52                           	pushl	%edx
  42d747: 50                           	pushl	%eax
  42d748: e8 f7 88 03 00               	calll	0x466044 <.text+0x65044>
  42d74d: 8b 54 24 20                  	movl	0x20(%esp), %edx
  42d751: 83 c4 0c                     	addl	$0xc, %esp
  42d754: 85 c0                        	testl	%eax, %eax
  42d756: 0f 95 c1                     	setne	%cl
  42d759: 88 8a 50 41 01 00            	movb	%cl, 0x14150(%edx)
  42d75f: e9 e6 02 00 00               	jmp	0x42da4a <.text+0x2ca4a>


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  42dd06: b9 c0 87 55 00               	movl	$0x5587c0, %ecx         # imm = 0x5587C0
  42dd0b: a3 c8 87 55 00               	movl	%eax, 0x5587c8
  42dd10: e8 cb ea ff ff               	calll	0x42c7e0 <.text+0x2b7e0>
  42dd15: b9 c0 87 55 00               	movl	$0x5587c0, %ecx         # imm = 0x5587C0
  42dd1a: e8 a1 c9 ff ff               	calll	0x42a6c0 <.text+0x296c0>


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  432c30: 55                           	pushl	%ebp
  432c31: 8b ec                        	movl	%esp, %ebp
  432c33: 83 e4 f8                     	andl	$-0x8, %esp
  432c36: 83 ec 74                     	subl	$0x74, %esp
  432c39: a1 48 ea 4b 00               	movl	0x4bea48, %eax
  432c3e: 33 c4                        	xorl	%esp, %eax
  432c40: 89 44 24 70                  	movl	%eax, 0x70(%esp)
  432c44: 8b 45 0c                     	movl	0xc(%ebp), %eax
  432c47: 85 c0                        	testl	%eax, %eax
  432c49: 8b 4d 20                     	movl	0x20(%ebp), %ecx
  432c4c: 8b 55 24                     	movl	0x24(%ebp), %edx
  432c4f: 53                           	pushl	%ebx
  432c50: 8b 5d 08                     	movl	0x8(%ebp), %ebx
  432c53: 56                           	pushl	%esi
  432c54: 8b 75 1c                     	movl	0x1c(%ebp), %esi
  432c57: 57                           	pushl	%edi
  432c58: 8b 7d 14                     	movl	0x14(%ebp), %edi
  432c5b: 89 44 24 1c                  	movl	%eax, 0x1c(%esp)
  432c5f: 89 7c 24 14                  	movl	%edi, 0x14(%esp)
  432c63: 89 74 24 10                  	movl	%esi, 0x10(%esp)
  432c67: 89 4c 24 18                  	movl	%ecx, 0x18(%esp)
  432c6b: 89 54 24 20                  	movl	%edx, 0x20(%esp)
  432c6f: 74 0f                        	je	0x432c80 <.text+0x31c80>
  432c71: 8b 4d 10                     	movl	0x10(%ebp), %ecx
  432c74: 51                           	pushl	%ecx
  432c75: 6a 00                        	pushl	$0x0
  432c77: 50                           	pushl	%eax
  432c78: e8 93 16 03 00               	calll	0x464310 <.text+0x63310>
  432c7d: 83 c4 0c                     	addl	$0xc, %esp
  432c80: 85 ff                        	testl	%edi, %edi
  432c82: 74 0f                        	je	0x432c93 <.text+0x31c93>
  432c84: 8b 55 18                     	movl	0x18(%ebp), %edx
  432c87: 52                           	pushl	%edx
  432c88: 6a 00                        	pushl	$0x0
  432c8a: 57                           	pushl	%edi
  432c8b: e8 80 16 03 00               	calll	0x464310 <.text+0x63310>
  432c90: 83 c4 0c                     	addl	$0xc, %esp
  432c93: 85 f6                        	testl	%esi, %esi
  432c95: 74 07                        	je	0x432c9e <.text+0x31c9e>
  432c97: 33 c0                        	xorl	%eax, %eax
  432c99: 89 06                        	movl	%eax, (%esi)
  432c9b: 89 46 04                     	movl	%eax, 0x4(%esi)
  432c9e: 6a 00                        	pushl	$0x0
  432ca0: 68 80 00 00 00               	pushl	$0x80
  432ca5: 6a 03                        	pushl	$0x3
  432ca7: 6a 00                        	pushl	$0x0
  432ca9: 6a 01                        	pushl	$0x1
  432cab: 68 00 00 00 80               	pushl	$0x80000000             # imm = 0x80000000
  432cb0: 53                           	pushl	%ebx
  432cb1: ff 15 e8 31 4a 00            	calll	*0x4a31e8
  432cb7: 8b f0                        	movl	%eax, %esi
  432cb9: 83 fe ff                     	cmpl	$-0x1, %esi
  432cbc: 0f 84 72 01 00 00            	je	0x432e34 <.text+0x31e34>
  432cc2: 6a 00                        	pushl	$0x0
  432cc4: 56                           	pushl	%esi
  432cc5: c6 44 24 17 01               	movb	$0x1, 0x17(%esp)
  432cca: ff 15 78 30 4a 00            	calll	*0x4a3078
  432cd0: 8b f8                        	movl	%eax, %edi
  432cd2: 57                           	pushl	%edi
  432cd3: e8 29 16 03 00               	calll	0x464301 <.text+0x63301>
  432cd8: 83 c4 04                     	addl	$0x4, %esp
  432cdb: 6a 00                        	pushl	$0x0
  432cdd: 8b d8                        	movl	%eax, %ebx
  432cdf: 8d 44 24 28                  	leal	0x28(%esp), %eax
  432ce3: 50                           	pushl	%eax
  432ce4: 57                           	pushl	%edi
  432ce5: 53                           	pushl	%ebx
  432ce6: 56                           	pushl	%esi
  432ce7: ff 15 30 31 4a 00            	calll	*0x4a3130
  432ced: 83 7c 24 10 00               	cmpl	$0x0, 0x10(%esp)
  432cf2: 74 24                        	je	0x432d18 <.text+0x31d18>
  432cf4: 8d 4c 24 28                  	leal	0x28(%esp), %ecx
  432cf8: 51                           	pushl	%ecx
  432cf9: 6a 00                        	pushl	$0x0
  432cfb: 6a 00                        	pushl	$0x0
  432cfd: 56                           	pushl	%esi
  432cfe: ff 15 c8 30 4a 00            	calll	*0x4a30c8
  432d04: 85 c0                        	testl	%eax, %eax
  432d06: 74 10                        	je	0x432d18 <.text+0x31d18>
  432d08: 8b 54 24 10                  	movl	0x10(%esp), %edx
  432d0c: 52                           	pushl	%edx
  432d0d: 8d 44 24 2c                  	leal	0x2c(%esp), %eax
  432d11: 50                           	pushl	%eax
  432d12: ff 15 c4 30 4a 00            	calll	*0x4a30c4
  432d18: 56                           	pushl	%esi
  432d19: ff 15 08 32 4a 00            	calll	*0x4a3208
  432d1f: 83 ff 48                     	cmpl	$0x48, %edi
  432d22: 0f 86 ed 00 00 00            	jbe	0x432e15 <.text+0x31e15>
  432d28: b9 12 00 00 00               	movl	$0x12, %ecx
  432d2d: 8b f3                        	movl	%ebx, %esi
  432d2f: 8d 7c 24 30                  	leal	0x30(%esp), %edi
  432d33: f3 a5                        	rep		movsl	(%esi), %es:(%edi)
  432d35: 8b 74 24 38                  	movl	0x38(%esp), %esi
  432d39: 6a 48                        	pushl	$0x48
  432d3b: 8d 4c 24 34                  	leal	0x34(%esp), %ecx
  432d3f: 6a 00                        	pushl	$0x0
  432d41: 51                           	pushl	%ecx
  432d42: e8 c9 15 03 00               	calll	0x464310 <.text+0x63310>
  432d47: 83 c4 0c                     	addl	$0xc, %esp
  432d4a: 83 fe 48                     	cmpl	$0x48, %esi
  432d4d: 77 11                        	ja	0x432d60 <.text+0x31d60>
  432d4f: 56                           	pushl	%esi
  432d50: 8d 54 24 34                  	leal	0x34(%esp), %edx
  432d54: 53                           	pushl	%ebx
  432d55: 52                           	pushl	%edx
  432d56: e8 75 0e 03 00               	calll	0x463bd0 <.text+0x62bd0>
  432d5b: 83 c4 0c                     	addl	$0xc, %esp
  432d5e: eb 0d                        	jmp	0x432d6d <.text+0x31d6d>
  432d60: b9 12 00 00 00               	movl	$0x12, %ecx
  432d65: 8b f3                        	movl	%ebx, %esi
  432d67: 8d 7c 24 30                  	leal	0x30(%esp), %edi
  432d6b: f3 a5                        	rep		movsl	(%esi), %es:(%edi)
  432d6d: 83 7c 24 44 00               	cmpl	$0x0, 0x44(%esp)
  432d72: 7e 23                        	jle	0x432d97 <.text+0x31d97>
  432d74: 8b 4c 24 1c                  	movl	0x1c(%esp), %ecx
  432d78: 85 c9                        	testl	%ecx, %ecx
  432d7a: 74 1b                        	je	0x432d97 <.text+0x31d97>
  432d7c: 8b 45 10                     	movl	0x10(%ebp), %eax
  432d7f: 85 c0                        	testl	%eax, %eax
  432d81: 7e 14                        	jle	0x432d97 <.text+0x31d97>
  432d83: 83 c0 ff                     	addl	$-0x1, %eax
  432d86: 50                           	pushl	%eax
  432d87: 8b 44 24 44                  	movl	0x44(%esp), %eax
  432d8b: 03 c3                        	addl	%ebx, %eax
  432d8d: 50                           	pushl	%eax
  432d8e: 51                           	pushl	%ecx
  432d8f: e8 bc 1f 03 00               	calll	0x464d50 <.text+0x63d50>
  432d94: 83 c4 0c                     	addl	$0xc, %esp
  432d97: 83 7c 24 74 00               	cmpl	$0x0, 0x74(%esp)
  432d9c: 8b 7c 24 14                  	movl	0x14(%esp), %edi
  432da0: 8b 75 18                     	movl	0x18(%ebp), %esi
  432da3: 7e 1c                        	jle	0x432dc1 <.text+0x31dc1>
  432da5: 85 ff                        	testl	%edi, %edi
  432da7: 74 18                        	je	0x432dc1 <.text+0x31dc1>
  432da9: 85 f6                        	testl	%esi, %esi
  432dab: 7e 14                        	jle	0x432dc1 <.text+0x31dc1>
  432dad: 8b 54 24 70                  	movl	0x70(%esp), %edx
  432db1: 8d 4e ff                     	leal	-0x1(%esi), %ecx
  432db4: 51                           	pushl	%ecx
  432db5: 03 d3                        	addl	%ebx, %edx
  432db7: 52                           	pushl	%edx
  432db8: 57                           	pushl	%edi
  432db9: e8 92 1f 03 00               	calll	0x464d50 <.text+0x63d50>
  432dbe: 83 c4 0c                     	addl	$0xc, %esp
  432dc1: 83 7c 24 4c 00               	cmpl	$0x0, 0x4c(%esp)
  432dc6: 7e 1c                        	jle	0x432de4 <.text+0x31de4>
  432dc8: 85 ff                        	testl	%edi, %edi
  432dca: 74 18                        	je	0x432de4 <.text+0x31de4>
  432dcc: 85 f6                        	testl	%esi, %esi
  432dce: 7e 14                        	jle	0x432de4 <.text+0x31de4>
  432dd0: 8b 44 24 48                  	movl	0x48(%esp), %eax
  432dd4: 83 c6 ff                     	addl	$-0x1, %esi
  432dd7: 56                           	pushl	%esi
  432dd8: 03 c3                        	addl	%ebx, %eax
  432dda: 50                           	pushl	%eax
  432ddb: 57                           	pushl	%edi
  432ddc: e8 6f 1f 03 00               	calll	0x464d50 <.text+0x63d50>
  432de1: 83 c4 0c                     	addl	$0xc, %esp
  432de4: 8b 44 24 64                  	movl	0x64(%esp), %eax
  432de8: 85 c0                        	testl	%eax, %eax
  432dea: 7e 29                        	jle	0x432e15 <.text+0x31e15>
  432dec: 8b 7c 24 18                  	movl	0x18(%esp), %edi
  432df0: 85 ff                        	testl	%edi, %edi
  432df2: 74 21                        	je	0x432e15 <.text+0x31e15>
  432df4: 8b 74 24 20                  	movl	0x20(%esp), %esi
  432df8: 50                           	pushl	%eax
  432df9: 89 06                        	movl	%eax, (%esi)
  432dfb: e8 01 15 03 00               	calll	0x464301 <.text+0x63301>
  432e00: 8b 54 24 64                  	movl	0x64(%esp), %edx
  432e04: 89 07                        	movl	%eax, (%edi)
  432e06: 8b 0e                        	movl	(%esi), %ecx
  432e08: 51                           	pushl	%ecx
  432e09: 03 d3                        	addl	%ebx, %edx
  432e0b: 52                           	pushl	%edx
  432e0c: 50                           	pushl	%eax
  432e0d: e8 be 0d 03 00               	calll	0x463bd0 <.text+0x62bd0>
  432e12: 83 c4 10                     	addl	$0x10, %esp
  432e15: 53                           	pushl	%ebx
  432e16: e8 e1 14 03 00               	calll	0x4642fc <.text+0x632fc>
  432e1b: 8a 44 24 13                  	movb	0x13(%esp), %al
  432e1f: 83 c4 04                     	addl	$0x4, %esp
  432e22: 5f                           	popl	%edi
  432e23: 5e                           	popl	%esi
  432e24: 5b                           	popl	%ebx
  432e25: 8b 4c 24 70                  	movl	0x70(%esp), %ecx
  432e29: 33 cc                        	xorl	%esp, %ecx
  432e2b: e8 8c 0d 03 00               	calll	0x463bbc <.text+0x62bbc>
  432e30: 8b e5                        	movl	%ebp, %esp
  432e32: 5d                           	popl	%ebp
  432e33: c3                           	retl
  432e34: 8b 4c 24 7c                  	movl	0x7c(%esp), %ecx
  432e38: 5f                           	popl	%edi
  432e39: 5e                           	popl	%esi
  432e3a: 5b                           	popl	%ebx
  432e3b: 33 cc                        	xorl	%esp, %ecx
  432e3d: 32 c0                        	xorb	%al, %al
  432e3f: e8 78 0d 03 00               	calll	0x463bbc <.text+0x62bbc>
  432e44: 8b e5                        	movl	%ebp, %esp
  432e46: 5d                           	popl	%ebp
  432e47: c3                           	retl
  432e48: cc                           	int3
  432e49: cc                           	int3
  432e4a: cc                           	int3
  432e4b: cc                           	int3
  432e4c: cc                           	int3
  432e4d: cc                           	int3
  432e4e: cc                           	int3
  432e4f: cc                           	int3
  432e50: 83 ec 64                     	subl	$0x64, %esp
  432e53: a1 48 ea 4b 00               	movl	0x4bea48, %eax
  432e58: 33 c4                        	xorl	%esp, %eax
  432e5a: 89 44 24 60                  	movl	%eax, 0x60(%esp)
  432e5e: 8b 44 24 6c                  	movl	0x6c(%esp), %eax
  432e62: 53                           	pushl	%ebx
  432e63: 8b 1d e8 31 4a 00            	movl	0x4a31e8, %ebx
  432e69: 55                           	pushl	%ebp
  432e6a: 56                           	pushl	%esi
  432e6b: 57                           	pushl	%edi
  432e6c: 8b 7c 24 78                  	movl	0x78(%esp), %edi
  432e70: 89 44 24 18                  	movl	%eax, 0x18(%esp)
  432e74: c6 44 24 13 00               	movb	$0x0, 0x13(%esp)
  432e79: 33 f6                        	xorl	%esi, %esi
  432e7b: eb 03                        	jmp	0x432e80 <.text+0x31e80>
  432e7d: 8d 49 00                     	leal	(%ecx), %ecx
  432e80: 6a 00                        	pushl	$0x0
  432e82: 68 80 00 00 00               	pushl	$0x80
  432e87: 6a 03                        	pushl	$0x3
  432e89: 6a 00                        	pushl	$0x0
  432e8b: 6a 00                        	pushl	$0x0
  432e8d: 68 00 00 00 c0               	pushl	$0xc0000000             # imm = 0xC0000000
  432e92: 57                           	pushl	%edi
  432e93: ff d3                        	calll	*%ebx
  432e95: 8b e8                        	movl	%eax, %ebp
  432e97: 83 fd ff                     	cmpl	$-0x1, %ebp
  432e9a: 75 2a                        	jne	0x432ec6 <.text+0x31ec6>
  432e9c: 6a 0a                        	pushl	$0xa
  432e9e: 83 c6 01                     	addl	$0x1, %esi
  432ea1: ff 15 ec 31 4a 00            	calll	*0x4a31ec
  432ea7: 81 fe e8 03 00 00            	cmpl	$0x3e8, %esi            # imm = 0x3E8
  432ead: 7e d1                        	jle	0x432e80 <.text+0x31e80>
  432eaf: 8a 44 24 13                  	movb	0x13(%esp), %al
  432eb3: 5f                           	popl	%edi
  432eb4: 5e                           	popl	%esi
  432eb5: 5d                           	popl	%ebp
  432eb6: 5b                           	popl	%ebx
  432eb7: 8b 4c 24 60                  	movl	0x60(%esp), %ecx
  432ebb: 33 cc                        	xorl	%esp, %ecx
  432ebd: e8 fa 0c 03 00               	calll	0x463bbc <.text+0x62bbc>
  432ec2: 83 c4 64                     	addl	$0x64, %esp
  432ec5: c3                           	retl
  432ec6: 8d 4c 24 20                  	leal	0x20(%esp), %ecx
  432eca: 51                           	pushl	%ecx
  432ecb: 6a 00                        	pushl	$0x0
  432ecd: 6a 00                        	pushl	$0x0
  432ecf: 55                           	pushl	%ebp
  432ed0: ff 15 c8 30 4a 00            	calll	*0x4a30c8
  432ed6: 6a 00                        	pushl	$0x0
  432ed8: 8d 54 24 18                  	leal	0x18(%esp), %edx
  432edc: 52                           	pushl	%edx
  432edd: 6a 48                        	pushl	$0x48
  432edf: 8d 44 24 34                  	leal	0x34(%esp), %eax
  432ee3: 50                           	pushl	%eax
  432ee4: 55                           	pushl	%ebp
  432ee5: ff 15 30 31 4a 00            	calll	*0x4a3130
  432eeb: 83 7c 24 14 48               	cmpl	$0x48, 0x14(%esp)
  432ef0: 72 73                        	jb	0x432f65 <.text+0x31f65>
  432ef2: bf 40 64 4a 00               	movl	$0x4a6440, %edi         # imm = 0x4A6440
  432ef7: 8d 74 24 28                  	leal	0x28(%esp), %esi
  432efb: b9 07 00 00 00               	movl	$0x7, %ecx
  432f00: 33 d2                        	xorl	%edx, %edx
  432f02: f3 a6                        	rep		cmpsb	%es:(%edi), (%esi)
  432f04: 75 5f                        	jne	0x432f65 <.text+0x31f65>
  432f06: 8b 3d fc 31 4a 00            	movl	0x4a31fc, %edi
  432f0c: 6a 02                        	pushl	$0x2
  432f0e: 52                           	pushl	%edx
  432f0f: 52                           	pushl	%edx
  432f10: 55                           	pushl	%ebp
  432f11: ff d7                        	calll	*%edi
  432f13: 8b 74 24 18                  	movl	0x18(%esp), %esi
  432f17: 89 44 24 38                  	movl	%eax, 0x38(%esp)
  432f1b: 8b c6                        	movl	%esi, %eax
  432f1d: 8d 48 01                     	leal	0x1(%eax), %ecx
  432f20: 8a 10                        	movb	(%eax), %dl
  432f22: 83 c0 01                     	addl	$0x1, %eax
  432f25: 84 d2                        	testb	%dl, %dl
  432f27: 75 f7                        	jne	0x432f20 <.text+0x31f20>
  432f29: 6a 00                        	pushl	$0x0
  432f2b: 2b c1                        	subl	%ecx, %eax
  432f2d: 8d 4c 24 20                  	leal	0x20(%esp), %ecx
  432f31: 51                           	pushl	%ecx
  432f32: 83 c0 01                     	addl	$0x1, %eax
  432f35: 50                           	pushl	%eax
  432f36: 56                           	pushl	%esi
  432f37: 8b 35 04 32 4a 00            	movl	0x4a3204, %esi
  432f3d: 55                           	pushl	%ebp
  432f3e: 89 44 24 50                  	movl	%eax, 0x50(%esp)
  432f42: ff d6                        	calll	*%esi
  432f44: 6a 00                        	pushl	$0x0
  432f46: 6a 00                        	pushl	$0x0
  432f48: 6a 00                        	pushl	$0x0
  432f4a: 55                           	pushl	%ebp
  432f4b: ff d7                        	calll	*%edi
  432f4d: 8b 44 24 14                  	movl	0x14(%esp), %eax
  432f51: 6a 00                        	pushl	$0x0
  432f53: 8d 54 24 20                  	leal	0x20(%esp), %edx
  432f57: 52                           	pushl	%edx
  432f58: 50                           	pushl	%eax
  432f59: 8d 4c 24 34                  	leal	0x34(%esp), %ecx
  432f5d: 51                           	pushl	%ecx
  432f5e: 55                           	pushl	%ebp
  432f5f: ff d6                        	calll	*%esi
  432f61: b3 01                        	movb	$0x1, %bl
  432f63: eb 04                        	jmp	0x432f69 <.text+0x31f69>
  432f65: 8a 5c 24 13                  	movb	0x13(%esp), %bl
  432f69: 8d 54 24 20                  	leal	0x20(%esp), %edx
  432f6d: 52                           	pushl	%edx
  432f6e: 6a 00                        	pushl	$0x0
  432f70: 6a 00                        	pushl	$0x0
  432f72: 55                           	pushl	%ebp
  432f73: ff 15 00 31 4a 00            	calll	*0x4a3100
  432f79: 55                           	pushl	%ebp
  432f7a: ff 15 08 32 4a 00            	calll	*0x4a3208
  432f80: 8b 4c 24 70                  	movl	0x70(%esp), %ecx
  432f84: 5f                           	popl	%edi
  432f85: 5e                           	popl	%esi
  432f86: 5d                           	popl	%ebp
  432f87: 8a c3                        	movb	%bl, %al
  432f89: 5b                           	popl	%ebx
  432f8a: 33 cc                        	xorl	%esp, %ecx
  432f8c: e8 2b 0c 03 00               	calll	0x463bbc <.text+0x62bbc>
  432f91: 83 c4 64                     	addl	$0x64, %esp
  432f94: c3                           	retl
  432f95: cc                           	int3
  432f96: cc                           	int3
  432f97: cc                           	int3
  432f98: cc                           	int3
  432f99: cc                           	int3
  432f9a: cc                           	int3
  432f9b: cc                           	int3
  432f9c: cc                           	int3
  432f9d: cc                           	int3
  432f9e: cc                           	int3
  432f9f: cc                           	int3


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  442970: 6a ff                        	pushl	$-0x1
  442972: 68 5b 1f 4a 00               	pushl	$0x4a1f5b               # imm = 0x4A1F5B
  442977: 64 a1 00 00 00 00            	movl	%fs:0x0, %eax
  44297d: 50                           	pushl	%eax
  44297e: 81 ec d4 02 00 00            	subl	$0x2d4, %esp            # imm = 0x2D4
  442984: a1 48 ea 4b 00               	movl	0x4bea48, %eax
  442989: 33 c4                        	xorl	%esp, %eax
  44298b: 89 84 24 d0 02 00 00         	movl	%eax, 0x2d0(%esp)
  442992: 53                           	pushl	%ebx
  442993: 55                           	pushl	%ebp
  442994: 56                           	pushl	%esi
  442995: 57                           	pushl	%edi
  442996: a1 48 ea 4b 00               	movl	0x4bea48, %eax
  44299b: 33 c4                        	xorl	%esp, %eax
  44299d: 50                           	pushl	%eax
  44299e: 8d 84 24 e8 02 00 00         	leal	0x2e8(%esp), %eax
  4429a5: 64 a3 00 00 00 00            	movl	%eax, %fs:0x0
  4429ab: 8b 9c 24 fc 02 00 00         	movl	0x2fc(%esp), %ebx
  4429b2: 8b 84 24 00 03 00 00         	movl	0x300(%esp), %eax
  4429b9: 8b bc 24 f8 02 00 00         	movl	0x2f8(%esp), %edi
  4429c0: 53                           	pushl	%ebx
  4429c1: 89 7c 24 30                  	movl	%edi, 0x30(%esp)
  4429c5: 89 5c 24 24                  	movl	%ebx, 0x24(%esp)
  4429c9: 89 44 24 18                  	movl	%eax, 0x18(%esp)
  4429cd: e8 2e 00 ff ff               	calll	0x432a00 <.text+0x31a00>
  4429d2: 53                           	pushl	%ebx
  4429d3: e8 38 00 ff ff               	calll	0x432a10 <.text+0x31a10>
  4429d8: 53                           	pushl	%ebx
  4429d9: e8 42 00 ff ff               	calll	0x432a20 <.text+0x31a20>
  4429de: 53                           	pushl	%ebx
  4429df: e8 4c 00 ff ff               	calll	0x432a30 <.text+0x31a30>
  4429e4: 68 c0 44 4a 00               	pushl	$0x4a44c0               # imm = 0x4A44C0
  4429e9: 57                           	pushl	%edi
  4429ea: be 80 00 00 00               	movl	$0x80, %esi
  4429ef: e8 ac 37 02 00               	calll	0x4661a0 <.text+0x651a0>
  4429f4: 83 c4 18                     	addl	$0x18, %esp
  4429f7: 85 c0                        	testl	%eax, %eax
  4429f9: 74 05                        	je	0x442a00 <.text+0x41a00>
  4429fb: be 00 01 00 00               	movl	$0x100, %esi            # imm = 0x100
  442a00: bd 48 00 00 00               	movl	$0x48, %ebp
  442a05: 55                           	pushl	%ebp
  442a06: 8d 8c 24 9c 00 00 00         	leal	0x9c(%esp), %ecx
  442a0d: 6a 00                        	pushl	$0x0
  442a0f: 51                           	pushl	%ecx
  442a10: e8 fb 18 02 00               	calll	0x464310 <.text+0x63310>
  442a15: 8b 15 40 64 4a 00            	movl	0x4a6440, %edx
  442a1b: 66 a1 44 64 4a 00            	movw	0x4a6444, %ax
  442a21: 83 c4 0c                     	addl	$0xc, %esp
  442a24: 6a 00                        	pushl	$0x0
  442a26: 56                           	pushl	%esi
  442a27: 6a 02                        	pushl	$0x2
  442a29: 6a 00                        	pushl	$0x0
  442a2b: 6a 01                        	pushl	$0x1
  442a2d: 68 00 00 00 40               	pushl	$0x40000000             # imm = 0x40000000
  442a32: 57                           	pushl	%edi
  442a33: 89 ac 24 bc 00 00 00         	movl	%ebp, 0xbc(%esp)
  442a3a: 89 94 24 b4 00 00 00         	movl	%edx, 0xb4(%esp)
  442a41: 66 89 84 24 b8 00 00 00      	movw	%ax, 0xb8(%esp)
  442a49: 66 c7 84 24 ba 00 00 00 00 00	movw	$0x0, 0xba(%esp)
  442a53: ff 15 e8 31 4a 00            	calll	*0x4a31e8
  442a59: 8b f0                        	movl	%eax, %esi
  442a5b: 83 fe ff                     	cmpl	$-0x1, %esi
  442a5e: 0f 84 21 05 00 00            	je	0x442f85 <.text+0x41f85>
  442a64: 8b 3d fc 31 4a 00            	movl	0x4a31fc, %edi
  442a6a: 6a 01                        	pushl	$0x1
  442a6c: 6a 00                        	pushl	$0x0
  442a6e: 6a 00                        	pushl	$0x0
  442a70: 56                           	pushl	%esi
  442a71: ff d7                        	calll	*%edi
  442a73: 6a 00                        	pushl	$0x0
  442a75: 8d 4c 24 1c                  	leal	0x1c(%esp), %ecx
  442a79: 51                           	pushl	%ecx
  442a7a: 55                           	pushl	%ebp
  442a7b: 8d 94 24 a4 00 00 00         	leal	0xa4(%esp), %edx
  442a82: 52                           	pushl	%edx
  442a83: 89 ac 24 b0 00 00 00         	movl	%ebp, 0xb0(%esp)
  442a8a: 8b 2d 04 32 4a 00            	movl	0x4a3204, %ebp
  442a90: 56                           	pushl	%esi
  442a91: 89 44 24 3c                  	movl	%eax, 0x3c(%esp)
  442a95: ff d5                        	calll	*%ebp
  442a97: 68 00 01 00 00               	pushl	$0x100                  # imm = 0x100
  442a9c: 8d 84 24 e4 00 00 00         	leal	0xe4(%esp), %eax
  442aa3: 6a 00                        	pushl	$0x0
  442aa5: 50                           	pushl	%eax
  442aa6: e8 65 18 02 00               	calll	0x464310 <.text+0x63310>
  442aab: 8b 44 24 20                  	movl	0x20(%esp), %eax
  442aaf: 83 c4 0c                     	addl	$0xc, %esp
  442ab2: 85 c0                        	testl	%eax, %eax
  442ab4: 74 16                        	je	0x442acc <.text+0x41acc>
  442ab6: 68 ff 00 00 00               	pushl	$0xff
  442abb: 50                           	pushl	%eax
  442abc: 8d 8c 24 e8 00 00 00         	leal	0xe8(%esp), %ecx
  442ac3: 51                           	pushl	%ecx
  442ac4: e8 87 22 02 00               	calll	0x464d50 <.text+0x63d50>
  442ac9: 83 c4 0c                     	addl	$0xc, %esp
  442acc: 6a 01                        	pushl	$0x1
  442ace: 6a 00                        	pushl	$0x0
  442ad0: 6a 00                        	pushl	$0x0
  442ad2: 56                           	pushl	%esi
  442ad3: ff d7                        	calll	*%edi
  442ad5: 89 84 24 a8 00 00 00         	movl	%eax, 0xa8(%esp)
  442adc: 8d 84 24 e0 00 00 00         	leal	0xe0(%esp), %eax
  442ae3: 8d 50 01                     	leal	0x1(%eax), %edx
  442ae6: 8a 08                        	movb	(%eax), %cl
  442ae8: 83 c0 01                     	addl	$0x1, %eax
  442aeb: 84 c9                        	testb	%cl, %cl
  442aed: 75 f7                        	jne	0x442ae6 <.text+0x41ae6>
  442aef: 2b c2                        	subl	%edx, %eax
  442af1: 6a 00                        	pushl	$0x0
  442af3: 83 c0 01                     	addl	$0x1, %eax
  442af6: 8d 54 24 1c                  	leal	0x1c(%esp), %edx
  442afa: 52                           	pushl	%edx
  442afb: 50                           	pushl	%eax
  442afc: 89 84 24 b8 00 00 00         	movl	%eax, 0xb8(%esp)
  442b03: 8d 84 24 ec 00 00 00         	leal	0xec(%esp), %eax
  442b0a: 50                           	pushl	%eax
  442b0b: 56                           	pushl	%esi
  442b0c: ff d5                        	calll	*%ebp
  442b0e: 83 bb 0c 01 08 00 00         	cmpl	$0x0, 0x8010c(%ebx)
  442b15: 74 42                        	je	0x442b59 <.text+0x41b59>
  442b17: 6a 01                        	pushl	$0x1
  442b19: 6a 00                        	pushl	$0x0
  442b1b: 6a 00                        	pushl	$0x0
  442b1d: 56                           	pushl	%esi
  442b1e: ff d7                        	calll	*%edi
  442b20: 8b 8b 0c 01 08 00            	movl	0x8010c(%ebx), %ecx
  442b26: 89 84 24 b0 00 00 00         	movl	%eax, 0xb0(%esp)
  442b2d: 8b c1                        	movl	%ecx, %eax
  442b2f: 8d 50 01                     	leal	0x1(%eax), %edx
  442b32: 89 54 24 14                  	movl	%edx, 0x14(%esp)
  442b36: 8a 10                        	movb	(%eax), %dl
  442b38: 83 c0 01                     	addl	$0x1, %eax
  442b3b: 84 d2                        	testb	%dl, %dl
  442b3d: 75 f7                        	jne	0x442b36 <.text+0x41b36>
  442b3f: 2b 44 24 14                  	subl	0x14(%esp), %eax
  442b43: 6a 00                        	pushl	$0x0
  442b45: 8d 54 24 1c                  	leal	0x1c(%esp), %edx
  442b49: 52                           	pushl	%edx
  442b4a: 83 c0 01                     	addl	$0x1, %eax
  442b4d: 50                           	pushl	%eax
  442b4e: 51                           	pushl	%ecx
  442b4f: 56                           	pushl	%esi
  442b50: 89 84 24 c8 00 00 00         	movl	%eax, 0xc8(%esp)
  442b57: ff d5                        	calll	*%ebp
  442b59: 83 3d 64 c9 56 00 10         	cmpl	$0x10, 0x56c964
  442b60: 8b 0d 50 c9 56 00            	movl	0x56c950, %ecx
  442b66: 73 05                        	jae	0x442b6d <.text+0x41b6d>
  442b68: b9 50 c9 56 00               	movl	$0x56c950, %ecx         # imm = 0x56C950
  442b6d: 8d 94 24 e0 00 00 00         	leal	0xe0(%esp), %edx
  442b74: 8a 01                        	movb	(%ecx), %al
  442b76: 88 02                        	movb	%al, (%edx)
  442b78: 83 c1 01                     	addl	$0x1, %ecx
  442b7b: 83 c2 01                     	addl	$0x1, %edx
  442b7e: 84 c0                        	testb	%al, %al
  442b80: 75 f2                        	jne	0x442b74 <.text+0x41b74>
  442b82: 6a 01                        	pushl	$0x1
  442b84: 6a 00                        	pushl	$0x0
  442b86: 6a 00                        	pushl	$0x0
  442b88: 56                           	pushl	%esi
  442b89: ff d7                        	calll	*%edi
  442b8b: 89 84 24 d8 00 00 00         	movl	%eax, 0xd8(%esp)
  442b92: 8d 84 24 e0 00 00 00         	leal	0xe0(%esp), %eax
  442b99: 8d 48 01                     	leal	0x1(%eax), %ecx
  442b9c: 8d 64 24 00                  	leal	(%esp), %esp
  442ba0: 8a 10                        	movb	(%eax), %dl
  442ba2: 83 c0 01                     	addl	$0x1, %eax
  442ba5: 84 d2                        	testb	%dl, %dl
  442ba7: 75 f7                        	jne	0x442ba0 <.text+0x41ba0>
  442ba9: 6a 00                        	pushl	$0x0
  442bab: 2b c1                        	subl	%ecx, %eax
  442bad: 8d 4c 24 1c                  	leal	0x1c(%esp), %ecx
  442bb1: 51                           	pushl	%ecx
  442bb2: 83 c0 01                     	addl	$0x1, %eax
  442bb5: 50                           	pushl	%eax
  442bb6: 8d 94 24 ec 00 00 00         	leal	0xec(%esp), %edx
  442bbd: 52                           	pushl	%edx
  442bbe: 56                           	pushl	%esi
  442bbf: 89 84 24 f0 00 00 00         	movl	%eax, 0xf0(%esp)


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  438780: 6a 0c                        	pushl	$0xc
  438782: e8 ea b2 02 00               	calll	0x463a71 <.text+0x62a71>
  438787: 83 c4 04                     	addl	$0x4, %esp
  43878a: 85 c0                        	testl	%eax, %eax
  43878c: 74 16                        	je	0x4387a4 <.text+0x377a4>
  43878e: c7 00 00 00 00 00            	movl	$0x0, (%eax)
  438794: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%eax)
  43879b: c7 40 08 00 00 00 00         	movl	$0x0, 0x8(%eax)
  4387a2: eb 02                        	jmp	0x4387a6 <.text+0x377a6>
  4387a4: 33 c0                        	xorl	%eax, %eax
  4387a6: 83 7c 24 08 01               	cmpl	$0x1, 0x8(%esp)
  4387ab: c7 00 01 00 00 00            	movl	$0x1, (%eax)
  4387b1: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%eax)
  4387b8: 7c 1d                        	jl	0x4387d7 <.text+0x377d7>
  4387ba: 8b 4c 24 0c                  	movl	0xc(%esp), %ecx
  4387be: 8b 09                        	movl	(%ecx), %ecx
  4387c0: 85 c9                        	testl	%ecx, %ecx
  4387c2: 74 13                        	je	0x4387d7 <.text+0x377d7>
  4387c4: 56                           	pushl	%esi
  4387c5: 8d 71 01                     	leal	0x1(%ecx), %esi
  4387c8: 8a 11                        	movb	(%ecx), %dl
  4387ca: 83 c1 01                     	addl	$0x1, %ecx
  4387cd: 84 d2                        	testb	%dl, %dl
  4387cf: 75 f7                        	jne	0x4387c8 <.text+0x377c8>
  4387d1: 2b ce                        	subl	%esi, %ecx
  4387d3: 89 48 04                     	movl	%ecx, 0x4(%eax)
  4387d6: 5e                           	popl	%esi
  4387d7: c3                           	retl
  4387d8: cc                           	int3
  4387d9: cc                           	int3
  4387da: cc                           	int3
  4387db: cc                           	int3
  4387dc: cc                           	int3
  4387dd: cc                           	int3
  4387de: cc                           	int3
  4387df: cc                           	int3


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  438940: 56                           	pushl	%esi
  438941: 33 f6                        	xorl	%esi, %esi
  438943: 33 c0                        	xorl	%eax, %eax
  438945: 83 7c 24 0c 01               	cmpl	$0x1, 0xc(%esp)
  43894a: 75 4f                        	jne	0x43899b <.text+0x3799b>
  43894c: 8b 44 24 10                  	movl	0x10(%esp), %eax
  438950: 57                           	pushl	%edi
  438951: 8b 38                        	movl	(%eax), %edi
  438953: 6a 0c                        	pushl	$0xc
  438955: e8 17 b1 02 00               	calll	0x463a71 <.text+0x62a71>
  43895a: 83 c4 04                     	addl	$0x4, %esp
  43895d: 3b c6                        	cmpl	%esi, %eax
  43895f: 74 0a                        	je	0x43896b <.text+0x3796b>
  438961: 89 30                        	movl	%esi, (%eax)
  438963: 89 70 04                     	movl	%esi, 0x4(%eax)
  438966: 89 70 08                     	movl	%esi, 0x8(%eax)
  438969: 8b f0                        	movl	%eax, %esi
  43896b: c7 06 01 00 00 00            	movl	$0x1, (%esi)
  438971: db 07                        	fildl	(%edi)
  438973: dc 0d 50 39 4a 00            	fmull	0x4a3950
  438979: dc 35 98 65 4a 00            	fdivl	0x4a6598
  43897f: e8 3c c9 02 00               	calll	0x4652c0 <.text+0x642c0>
  438984: dc 0d f8 34 4a 00            	fmull	0x4a34f8
  43898a: dc 05 90 65 4a 00            	faddl	0x4a6590
  438990: e8 0b c3 02 00               	calll	0x464ca0 <.text+0x63ca0>
  438995: 89 46 04                     	movl	%eax, 0x4(%esi)
  438998: 8b c6                        	movl	%esi, %eax
  43899a: 5f                           	popl	%edi
  43899b: 5e                           	popl	%esi
  43899c: c3                           	retl
  43899d: cc                           	int3
  43899e: cc                           	int3
  43899f: cc                           	int3
  4389a0: 56                           	pushl	%esi
  4389a1: 33 f6                        	xorl	%esi, %esi
  4389a3: 33 c0                        	xorl	%eax, %eax
  4389a5: 83 7c 24 0c 01               	cmpl	$0x1, 0xc(%esp)
  4389aa: 75 4f                        	jne	0x4389fb <.text+0x379fb>
  4389ac: 8b 44 24 10                  	movl	0x10(%esp), %eax
  4389b0: 57                           	pushl	%edi
  4389b1: 8b 38                        	movl	(%eax), %edi
  4389b3: 6a 0c                        	pushl	$0xc
  4389b5: e8 b7 b0 02 00               	calll	0x463a71 <.text+0x62a71>
  4389ba: 83 c4 04                     	addl	$0x4, %esp
  4389bd: 3b c6                        	cmpl	%esi, %eax
  4389bf: 74 0a                        	je	0x4389cb <.text+0x379cb>
  4389c1: 89 30                        	movl	%esi, (%eax)
  4389c3: 89 70 04                     	movl	%esi, 0x4(%eax)
  4389c6: 89 70 08                     	movl	%esi, 0x8(%eax)
  4389c9: 8b f0                        	movl	%eax, %esi
  4389cb: c7 06 01 00 00 00            	movl	$0x1, (%esi)
  4389d1: db 07                        	fildl	(%edi)
  4389d3: dc 0d 50 39 4a 00            	fmull	0x4a3950
  4389d9: dc 35 98 65 4a 00            	fdivl	0x4a6598
  4389df: e8 ac c7 02 00               	calll	0x465190 <.text+0x64190>
  4389e4: dc 0d f8 34 4a 00            	fmull	0x4a34f8
  4389ea: dc 05 90 65 4a 00            	faddl	0x4a6590
  4389f0: e8 ab c2 02 00               	calll	0x464ca0 <.text+0x63ca0>
  4389f5: 89 46 04                     	movl	%eax, 0x4(%esi)
  4389f8: 8b c6                        	movl	%esi, %eax
  4389fa: 5f                           	popl	%edi
  4389fb: 5e                           	popl	%esi
  4389fc: c3                           	retl
  4389fd: cc                           	int3
  4389fe: cc                           	int3
  4389ff: cc                           	int3
  438a00: 56                           	pushl	%esi
  438a01: 33 f6                        	xorl	%esi, %esi
  438a03: 33 c0                        	xorl	%eax, %eax
  438a05: 83 7c 24 0c 01               	cmpl	$0x1, 0xc(%esp)
  438a0a: 75 4f                        	jne	0x438a5b <.text+0x37a5b>
  438a0c: 8b 44 24 10                  	movl	0x10(%esp), %eax
  438a10: 57                           	pushl	%edi
  438a11: 8b 38                        	movl	(%eax), %edi
  438a13: 6a 0c                        	pushl	$0xc
  438a15: e8 57 b0 02 00               	calll	0x463a71 <.text+0x62a71>
  438a1a: 83 c4 04                     	addl	$0x4, %esp
  438a1d: 3b c6                        	cmpl	%esi, %eax
  438a1f: 74 0a                        	je	0x438a2b <.text+0x37a2b>
  438a21: 89 30                        	movl	%esi, (%eax)
  438a23: 89 70 04                     	movl	%esi, 0x4(%eax)
  438a26: 89 70 08                     	movl	%esi, 0x8(%eax)
  438a29: 8b f0                        	movl	%eax, %esi
  438a2b: c7 06 01 00 00 00            	movl	$0x1, (%esi)
  438a31: db 07                        	fildl	(%edi)
  438a33: dc 0d 50 39 4a 00            	fmull	0x4a3950
  438a39: dc 35 98 65 4a 00            	fdivl	0x4a6598
  438a3f: e8 6c d6 02 00               	calll	0x4660b0 <.text+0x650b0>
  438a44: dc 0d f8 34 4a 00            	fmull	0x4a34f8
  438a4a: dc 05 90 65 4a 00            	faddl	0x4a6590
  438a50: e8 4b c2 02 00               	calll	0x464ca0 <.text+0x63ca0>
  438a55: 89 46 04                     	movl	%eax, 0x4(%esi)
  438a58: 8b c6                        	movl	%esi, %eax
  438a5a: 5f                           	popl	%edi
  438a5b: 5e                           	popl	%esi
  438a5c: c3                           	retl
  438a5d: cc                           	int3
  438a5e: cc                           	int3
  438a5f: cc                           	int3


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  437be0: 83 7c 24 08 01               	cmpl	$0x1, 0x8(%esp)
  437be5: 75 25                        	jne	0x437c0c <.text+0x36c0c>
  437be7: 8b 44 24 0c                  	movl	0xc(%esp), %eax
  437beb: 56                           	pushl	%esi
  437bec: 8b 30                        	movl	(%eax), %esi
  437bee: 8b 06                        	movl	(%esi), %eax
  437bf0: 85 c0                        	testl	%eax, %eax
  437bf2: 7c 14                        	jl	0x437c08 <.text+0x36c08>
  437bf4: 83 f8 10                     	cmpl	$0x10, %eax
  437bf7: 73 0f                        	jae	0x437c08 <.text+0x36c08>
  437bf9: ff 15 a0 33 4a 00            	calll	*0x4a33a0
  437bff: 8b 0e                        	movl	(%esi), %ecx
  437c01: 89 04 8d c4 c3 56 00         	movl	%eax, 0x56c3c4(,%ecx,4)
  437c08: 33 c0                        	xorl	%eax, %eax
  437c0a: 5e                           	popl	%esi
  437c0b: c3                           	retl
  437c0c: 33 c0                        	xorl	%eax, %eax
  437c0e: c3                           	retl
  437c0f: cc                           	int3
  437c10: 83 7c 24 08 02               	cmpl	$0x2, 0x8(%esp)
  437c15: 0f 85 9f 00 00 00            	jne	0x437cba <.text+0x36cba>
  437c1b: 8b 44 24 0c                  	movl	0xc(%esp), %eax
  437c1f: 56                           	pushl	%esi
  437c20: 8b 30                        	movl	(%eax), %esi
  437c22: 57                           	pushl	%edi
  437c23: 8b 78 04                     	movl	0x4(%eax), %edi
  437c26: 8b 06                        	movl	(%esi), %eax
  437c28: 85 c0                        	testl	%eax, %eax
  437c2a: 0f 8c 88 00 00 00            	jl	0x437cb8 <.text+0x36cb8>
  437c30: 83 f8 10                     	cmpl	$0x10, %eax
  437c33: 0f 83 7f 00 00 00            	jae	0x437cb8 <.text+0x36cb8>
  437c39: 83 3c 85 c4 c3 56 00 00      	cmpl	$0x0, 0x56c3c4(,%eax,4)
  437c41: 74 75                        	je	0x437cb8 <.text+0x36cb8>
  437c43: 55                           	pushl	%ebp
  437c44: 8b 2d a0 33 4a 00            	movl	0x4a33a0, %ebp
  437c4a: ff d5                        	calll	*%ebp
  437c4c: 8b 0e                        	movl	(%esi), %ecx
  437c4e: 2b 04 8d c4 c3 56 00         	subl	0x56c3c4(,%ecx,4), %eax
  437c55: 8b 0f                        	movl	(%edi), %ecx
  437c57: 3b c8                        	cmpl	%eax, %ecx
  437c59: 76 5c                        	jbe	0x437cb7 <.text+0x36cb7>
  437c5b: 53                           	pushl	%ebx
  437c5c: 8b 1d ec 31 4a 00            	movl	0x4a31ec, %ebx
  437c62: 2b c8                        	subl	%eax, %ecx
  437c64: 83 f9 64                     	cmpl	$0x64, %ecx
  437c67: 76 04                        	jbe	0x437c6d <.text+0x36c6d>
  437c69: 6a 64                        	pushl	$0x64
  437c6b: eb 01                        	jmp	0x437c6e <.text+0x36c6e>
  437c6d: 51                           	pushl	%ecx
  437c6e: ff d3                        	calll	*%ebx
  437c70: 8b 54 24 14                  	movl	0x14(%esp), %edx
  437c74: 52                           	pushl	%edx
  437c75: e8 76 bc ff ff               	calll	0x4338f0 <.text+0x328f0>
  437c7a: 83 c4 04                     	addl	$0x4, %esp
  437c7d: 84 c0                        	testb	%al, %al
  437c7f: 75 35                        	jne	0x437cb6 <.text+0x36cb6>
  437c81: 80 3d 40 c3 56 00 01         	cmpb	$0x1, 0x56c340
  437c88: 74 2c                        	je	0x437cb6 <.text+0x36cb6>
  437c8a: 80 3d 48 c3 56 00 01         	cmpb	$0x1, 0x56c348
  437c91: 74 23                        	je	0x437cb6 <.text+0x36cb6>
  437c93: 80 3d 49 c3 56 00 01         	cmpb	$0x1, 0x56c349
  437c9a: 74 1a                        	je	0x437cb6 <.text+0x36cb6>
  437c9c: 80 3d d2 87 55 00 01         	cmpb	$0x1, 0x5587d2
  437ca3: 74 11                        	je	0x437cb6 <.text+0x36cb6>
  437ca5: ff d5                        	calll	*%ebp
  437ca7: 8b 0e                        	movl	(%esi), %ecx
  437ca9: 2b 04 8d c4 c3 56 00         	subl	0x56c3c4(,%ecx,4), %eax
  437cb0: 8b 0f                        	movl	(%edi), %ecx
  437cb2: 3b c8                        	cmpl	%eax, %ecx
  437cb4: 77 ac                        	ja	0x437c62 <.text+0x36c62>
  437cb6: 5b                           	popl	%ebx
  437cb7: 5d                           	popl	%ebp
  437cb8: 5f                           	popl	%edi
  437cb9: 5e                           	popl	%esi
  437cba: 33 c0                        	xorl	%eax, %eax
  437cbc: c3                           	retl
  437cbd: cc                           	int3
  437cbe: cc                           	int3
  437cbf: cc                           	int3
  437cc0: 56                           	pushl	%esi
  437cc1: 6a 0c                        	pushl	$0xc
  437cc3: e8 a9 bd 02 00               	calll	0x463a71 <.text+0x62a71>
  437cc8: 33 c9                        	xorl	%ecx, %ecx
  437cca: 83 c4 04                     	addl	$0x4, %esp
  437ccd: 3b c1                        	cmpl	%ecx, %eax
  437ccf: 74 0c                        	je	0x437cdd <.text+0x36cdd>
  437cd1: 89 08                        	movl	%ecx, (%eax)
  437cd3: 89 48 04                     	movl	%ecx, 0x4(%eax)
  437cd6: 89 48 08                     	movl	%ecx, 0x8(%eax)
  437cd9: 8b f0                        	movl	%eax, %esi
  437cdb: eb 02                        	jmp	0x437cdf <.text+0x36cdf>
  437cdd: 33 f6                        	xorl	%esi, %esi
  437cdf: 83 7c 24 0c 01               	cmpl	$0x1, 0xc(%esp)
  437ce4: c7 06 01 00 00 00            	movl	$0x1, (%esi)
  437cea: 89 4e 04                     	movl	%ecx, 0x4(%esi)
  437ced: 75 32                        	jne	0x437d21 <.text+0x36d21>
  437cef: 8b 44 24 10                  	movl	0x10(%esp), %eax
  437cf3: 57                           	pushl	%edi
  437cf4: 8b 38                        	movl	(%eax), %edi
  437cf6: 8b 07                        	movl	(%edi), %eax
  437cf8: 3b c1                        	cmpl	%ecx, %eax
  437cfa: 7c 20                        	jl	0x437d1c <.text+0x36d1c>
  437cfc: 83 f8 10                     	cmpl	$0x10, %eax
  437cff: 73 1b                        	jae	0x437d1c <.text+0x36d1c>
  437d01: 39 0c 85 c4 c3 56 00         	cmpl	%ecx, 0x56c3c4(,%eax,4)
  437d08: 74 12                        	je	0x437d1c <.text+0x36d1c>
  437d0a: ff 15 a0 33 4a 00            	calll	*0x4a33a0
  437d10: 8b 0f                        	movl	(%edi), %ecx
  437d12: 2b 04 8d c4 c3 56 00         	subl	0x56c3c4(,%ecx,4), %eax
  437d19: 89 46 04                     	movl	%eax, 0x4(%esi)
  437d1c: 5f                           	popl	%edi
  437d1d: 8b c6                        	movl	%esi, %eax
  437d1f: 5e                           	popl	%esi
  437d20: c3                           	retl
  437d21: 8b c6                        	movl	%esi, %eax
  437d23: 5e                           	popl	%esi
  437d24: c3                           	retl
  437d25: cc                           	int3
  437d26: cc                           	int3
  437d27: cc                           	int3
  437d28: cc                           	int3
  437d29: cc                           	int3
  437d2a: cc                           	int3
  437d2b: cc                           	int3
  437d2c: cc                           	int3
  437d2d: cc                           	int3
  437d2e: cc                           	int3
  437d2f: cc                           	int3


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  43c2b0: 56                           	pushl	%esi
  43c2b1: 6a 0c                        	pushl	$0xc
  43c2b3: e8 b9 77 02 00               	calll	0x463a71 <.text+0x62a71>
  43c2b8: 33 c9                        	xorl	%ecx, %ecx
  43c2ba: 83 c4 04                     	addl	$0x4, %esp
  43c2bd: 3b c1                        	cmpl	%ecx, %eax
  43c2bf: 74 0c                        	je	0x43c2cd <.text+0x3b2cd>
  43c2c1: 89 08                        	movl	%ecx, (%eax)
  43c2c3: 89 48 04                     	movl	%ecx, 0x4(%eax)
  43c2c6: 89 48 08                     	movl	%ecx, 0x8(%eax)
  43c2c9: 8b f0                        	movl	%eax, %esi
  43c2cb: eb 02                        	jmp	0x43c2cf <.text+0x3b2cf>
  43c2cd: 33 f6                        	xorl	%esi, %esi
  43c2cf: 83 7c 24 0c 01               	cmpl	$0x1, 0xc(%esp)
  43c2d4: c7 06 01 00 00 00            	movl	$0x1, (%esi)
  43c2da: 89 4e 04                     	movl	%ecx, 0x4(%esi)
  43c2dd: 7c 1e                        	jl	0x43c2fd <.text+0x3b2fd>
  43c2df: 8b 44 24 10                  	movl	0x10(%esp), %eax
  43c2e3: 8b 00                        	movl	(%eax), %eax
  43c2e5: 50                           	pushl	%eax
  43c2e6: e8 45 d6 fe ff               	calll	0x429930 <.text+0x28930>
  43c2eb: 83 c4 04                     	addl	$0x4, %esp
  43c2ee: 3c 01                        	cmpb	$0x1, %al
  43c2f0: 8b c6                        	movl	%esi, %eax
  43c2f2: 75 0b                        	jne	0x43c2ff <.text+0x3b2ff>
  43c2f4: c7 46 04 01 00 00 00         	movl	$0x1, 0x4(%esi)
  43c2fb: 5e                           	popl	%esi
  43c2fc: c3                           	retl
  43c2fd: 8b c6                        	movl	%esi, %eax
  43c2ff: 5e                           	popl	%esi
  43c300: c3                           	retl
  43c301: cc                           	int3
  43c302: cc                           	int3
  43c303: cc                           	int3
  43c304: cc                           	int3
  43c305: cc                           	int3
  43c306: cc                           	int3
  43c307: cc                           	int3
  43c308: cc                           	int3
  43c309: cc                           	int3
  43c30a: cc                           	int3
  43c30b: cc                           	int3
  43c30c: cc                           	int3
  43c30d: cc                           	int3
  43c30e: cc                           	int3
  43c30f: cc                           	int3
  43c310: 56                           	pushl	%esi
  43c311: 6a 0c                        	pushl	$0xc
  43c313: e8 59 77 02 00               	calll	0x463a71 <.text+0x62a71>
  43c318: 33 c9                        	xorl	%ecx, %ecx
  43c31a: 83 c4 04                     	addl	$0x4, %esp
  43c31d: 3b c1                        	cmpl	%ecx, %eax
  43c31f: 74 0c                        	je	0x43c32d <.text+0x3b32d>
  43c321: 89 08                        	movl	%ecx, (%eax)
  43c323: 89 48 04                     	movl	%ecx, 0x4(%eax)
  43c326: 89 48 08                     	movl	%ecx, 0x8(%eax)
  43c329: 8b f0                        	movl	%eax, %esi
  43c32b: eb 02                        	jmp	0x43c32f <.text+0x3b32f>
  43c32d: 33 f6                        	xorl	%esi, %esi
  43c32f: 83 7c 24 0c 01               	cmpl	$0x1, 0xc(%esp)
  43c334: c7 06 01 00 00 00            	movl	$0x1, (%esi)
  43c33a: 89 4e 04                     	movl	%ecx, 0x4(%esi)
  43c33d: 7c 20                        	jl	0x43c35f <.text+0x3b35f>
  43c33f: 8b 44 24 10                  	movl	0x10(%esp), %eax
  43c343: 8b 00                        	movl	(%eax), %eax
  43c345: 51                           	pushl	%ecx
  43c346: 51                           	pushl	%ecx
  43c347: 50                           	pushl	%eax
  43c348: e8 a3 df fe ff               	calll	0x42a2f0 <.text+0x292f0>
  43c34d: 83 c4 0c                     	addl	$0xc, %esp
  43c350: 3c 01                        	cmpb	$0x1, %al
  43c352: 8b c6                        	movl	%esi, %eax
  43c354: 75 0b                        	jne	0x43c361 <.text+0x3b361>
  43c356: c7 46 04 01 00 00 00         	movl	$0x1, 0x4(%esi)
  43c35d: 5e                           	popl	%esi
  43c35e: c3                           	retl
  43c35f: 8b c6                        	movl	%esi, %eax
  43c361: 5e                           	popl	%esi
  43c362: c3                           	retl


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  43c3f0: 6a 0c                        	pushl	$0xc
  43c3f2: e8 7a 76 02 00               	calll	0x463a71 <.text+0x62a71>
  43c3f7: 33 c9                        	xorl	%ecx, %ecx
  43c3f9: 83 c4 04                     	addl	$0x4, %esp
  43c3fc: 3b c1                        	cmpl	%ecx, %eax
  43c3fe: 74 18                        	je	0x43c418 <.text+0x3b418>
  43c400: 89 08                        	movl	%ecx, (%eax)
  43c402: 89 48 04                     	movl	%ecx, 0x4(%eax)
  43c405: 89 48 08                     	movl	%ecx, 0x8(%eax)
  43c408: c7 00 01 00 00 00            	movl	$0x1, (%eax)
  43c40e: 8b 0d c4 70 55 00            	movl	0x5570c4, %ecx
  43c414: 89 48 04                     	movl	%ecx, 0x4(%eax)
  43c417: c3                           	retl
  43c418: 33 c0                        	xorl	%eax, %eax
  43c41a: c7 00 01 00 00 00            	movl	$0x1, (%eax)
  43c420: 8b 0d c4 70 55 00            	movl	0x5570c4, %ecx
  43c426: 89 48 04                     	movl	%ecx, 0x4(%eax)
  43c429: c3                           	retl
  43c42a: cc                           	int3
  43c42b: cc                           	int3
  43c42c: cc                           	int3
  43c42d: cc                           	int3
  43c42e: cc                           	int3
  43c42f: cc                           	int3
  43c430: 6a 0c                        	pushl	$0xc
  43c432: e8 3a 76 02 00               	calll	0x463a71 <.text+0x62a71>
  43c437: 33 c9                        	xorl	%ecx, %ecx
  43c439: 83 c4 04                     	addl	$0x4, %esp
  43c43c: 3b c1                        	cmpl	%ecx, %eax
  43c43e: 74 18                        	je	0x43c458 <.text+0x3b458>
  43c440: 89 08                        	movl	%ecx, (%eax)
  43c442: 89 48 04                     	movl	%ecx, 0x4(%eax)
  43c445: 89 48 08                     	movl	%ecx, 0x8(%eax)
  43c448: c7 00 01 00 00 00            	movl	$0x1, (%eax)
  43c44e: 8b 0d c0 70 55 00            	movl	0x5570c0, %ecx
  43c454: 89 48 04                     	movl	%ecx, 0x4(%eax)
  43c457: c3                           	retl
  43c458: 33 c0                        	xorl	%eax, %eax
  43c45a: c7 00 01 00 00 00            	movl	$0x1, (%eax)
  43c460: 8b 0d c0 70 55 00            	movl	0x5570c0, %ecx
  43c466: 89 48 04                     	movl	%ecx, 0x4(%eax)
  43c469: c3                           	retl
  43c46a: cc                           	int3
  43c46b: cc                           	int3
  43c46c: cc                           	int3
  43c46d: cc                           	int3
  43c46e: cc                           	int3
  43c46f: cc                           	int3


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  43c600: 6a 0c                        	pushl	$0xc
  43c602: e8 6a 74 02 00               	calll	0x463a71 <.text+0x62a71>
  43c607: 33 c9                        	xorl	%ecx, %ecx
  43c609: 83 c4 04                     	addl	$0x4, %esp
  43c60c: 3b c1                        	cmpl	%ecx, %eax
  43c60e: 74 12                        	je	0x43c622 <.text+0x3b622>
  43c610: 89 08                        	movl	%ecx, (%eax)
  43c612: 89 48 04                     	movl	%ecx, 0x4(%eax)
  43c615: 89 48 08                     	movl	%ecx, 0x8(%eax)
  43c618: c7 00 01 00 00 00            	movl	$0x1, (%eax)
  43c61e: 89 48 04                     	movl	%ecx, 0x4(%eax)
  43c621: c3                           	retl
  43c622: 33 c0                        	xorl	%eax, %eax
  43c624: c7 00 01 00 00 00            	movl	$0x1, (%eax)
  43c62a: 89 48 04                     	movl	%ecx, 0x4(%eax)
  43c62d: c3                           	retl
  43c62e: cc                           	int3
  43c62f: cc                           	int3
  43c630: 83 ec 24                     	subl	$0x24, %esp
  43c633: a1 48 ea 4b 00               	movl	0x4bea48, %eax
  43c638: 33 c4                        	xorl	%esp, %eax
  43c63a: 89 44 24 20                  	movl	%eax, 0x20(%esp)
  43c63e: 56                           	pushl	%esi
  43c63f: 57                           	pushl	%edi
  43c640: 8b 7c 24 38                  	movl	0x38(%esp), %edi
  43c644: 6a 0c                        	pushl	$0xc
  43c646: e8 26 74 02 00               	calll	0x463a71 <.text+0x62a71>
  43c64b: 33 c9                        	xorl	%ecx, %ecx
  43c64d: 83 c4 04                     	addl	$0x4, %esp
  43c650: 3b c1                        	cmpl	%ecx, %eax
  43c652: 74 0c                        	je	0x43c660 <.text+0x3b660>
  43c654: 89 08                        	movl	%ecx, (%eax)
  43c656: 89 48 04                     	movl	%ecx, 0x4(%eax)
  43c659: 89 48 08                     	movl	%ecx, 0x8(%eax)
  43c65c: 8b f0                        	movl	%eax, %esi
  43c65e: eb 02                        	jmp	0x43c662 <.text+0x3b662>
  43c660: 33 f6                        	xorl	%esi, %esi
  43c662: 83 7c 24 34 0b               	cmpl	$0xb, 0x34(%esp)
  43c667: c7 06 01 00 00 00            	movl	$0x1, (%esi)
  43c66d: 89 4e 04                     	movl	%ecx, 0x4(%esi)
  43c670: 89 4c 24 18                  	movl	%ecx, 0x18(%esp)
  43c674: 66 89 4c 24 1c               	movw	%cx, 0x1c(%esp)
  43c679: 66 89 4c 24 1e               	movw	%cx, 0x1e(%esp)
  43c67e: 88 4c 24 20                  	movb	%cl, 0x20(%esp)
  43c682: 88 4c 24 21                  	movb	%cl, 0x21(%esp)
  43c686: 88 4c 24 22                  	movb	%cl, 0x22(%esp)
  43c68a: 88 4c 24 23                  	movb	%cl, 0x23(%esp)
  43c68e: 88 4c 24 24                  	movb	%cl, 0x24(%esp)
  43c692: 88 4c 24 25                  	movb	%cl, 0x25(%esp)
  43c696: 88 4c 24 26                  	movb	%cl, 0x26(%esp)
  43c69a: 88 4c 24 27                  	movb	%cl, 0x27(%esp)
  43c69e: c7 44 24 08 0e 73 61 cb      	movl	$0xcb61730e, 0x8(%esp)  # imm = 0xCB61730E
  43c6a6: 66 c7 44 24 0c c4 36         	movw	$0x36c4, 0xc(%esp)      # imm = 0x36C4
  43c6ad: 66 c7 44 24 0e 9f 45         	movw	$0x459f, 0xe(%esp)      # imm = 0x459F
  43c6b4: c6 44 24 10 9d               	movb	$-0x63, 0x10(%esp)
  43c6b9: c6 44 24 11 b3               	movb	$-0x4d, 0x11(%esp)
  43c6be: c6 44 24 12 57               	movb	$0x57, 0x12(%esp)
  43c6c3: c6 44 24 13 f3               	movb	$-0xd, 0x13(%esp)
  43c6c8: c6 44 24 14 49               	movb	$0x49, 0x14(%esp)
  43c6cd: c6 44 24 15 eb               	movb	$-0x15, 0x15(%esp)
  43c6d2: c6 44 24 16 f9               	movb	$-0x7, 0x16(%esp)
  43c6d7: c6 44 24 17 e2               	movb	$-0x1e, 0x17(%esp)
  43c6dc: 75 6e                        	jne	0x43c74c <.text+0x3b74c>
  43c6de: 8b 07                        	movl	(%edi), %eax
  43c6e0: 8b 08                        	movl	(%eax), %ecx
  43c6e2: 8b 57 04                     	movl	0x4(%edi), %edx
  43c6e5: 66 8b 02                     	movw	(%edx), %ax
  43c6e8: 89 4c 24 08                  	movl	%ecx, 0x8(%esp)
  43c6ec: 8b 4f 08                     	movl	0x8(%edi), %ecx
  43c6ef: 66 8b 11                     	movw	(%ecx), %dx
  43c6f2: 66 89 44 24 0c               	movw	%ax, 0xc(%esp)
  43c6f7: 8b 47 0c                     	movl	0xc(%edi), %eax
  43c6fa: 0f b6 08                     	movzbl	(%eax), %ecx
  43c6fd: 66 89 54 24 0e               	movw	%dx, 0xe(%esp)
  43c702: 8b 57 10                     	movl	0x10(%edi), %edx
  43c705: 0f b6 02                     	movzbl	(%edx), %eax
  43c708: 88 4c 24 10                  	movb	%cl, 0x10(%esp)
  43c70c: 8b 4f 14                     	movl	0x14(%edi), %ecx
  43c70f: 0f b6 11                     	movzbl	(%ecx), %edx
  43c712: 88 44 24 11                  	movb	%al, 0x11(%esp)
  43c716: 8b 47 18                     	movl	0x18(%edi), %eax
  43c719: 0f b6 08                     	movzbl	(%eax), %ecx
  43c71c: 88 54 24 12                  	movb	%dl, 0x12(%esp)
  43c720: 8b 57 1c                     	movl	0x1c(%edi), %edx
  43c723: 0f b6 02                     	movzbl	(%edx), %eax
  43c726: 88 4c 24 13                  	movb	%cl, 0x13(%esp)
  43c72a: 8b 4f 20                     	movl	0x20(%edi), %ecx
  43c72d: 0f b6 11                     	movzbl	(%ecx), %edx
  43c730: 88 44 24 14                  	movb	%al, 0x14(%esp)
  43c734: 8b 47 24                     	movl	0x24(%edi), %eax
  43c737: 0f b6 08                     	movzbl	(%eax), %ecx
  43c73a: 88 54 24 15                  	movb	%dl, 0x15(%esp)
  43c73e: 8b 57 28                     	movl	0x28(%edi), %edx
  43c741: 0f b6 02                     	movzbl	(%edx), %eax
  43c744: 88 4c 24 16                  	movb	%cl, 0x16(%esp)
  43c748: 88 44 24 17                  	movb	%al, 0x17(%esp)
  43c74c: 8d 4c 24 08                  	leal	0x8(%esp), %ecx
  43c750: 51                           	pushl	%ecx
  43c751: 8d 54 24 1c                  	leal	0x1c(%esp), %edx
  43c755: 52                           	pushl	%edx
  43c756: e8 a5 5e ff ff               	calll	0x432600 <.text+0x31600>
  43c75b: 83 c4 08                     	addl	$0x8, %esp
  43c75e: 85 c0                        	testl	%eax, %eax
  43c760: 8b c6                        	movl	%esi, %eax
  43c762: 74 07                        	je	0x43c76b <.text+0x3b76b>
  43c764: c7 46 04 01 00 00 00         	movl	$0x1, 0x4(%esi)
  43c76b: 8b 4c 24 28                  	movl	0x28(%esp), %ecx
  43c76f: 5f                           	popl	%edi
  43c770: 5e                           	popl	%esi
  43c771: 33 cc                        	xorl	%esp, %ecx
  43c773: e8 44 74 02 00               	calll	0x463bbc <.text+0x62bbc>
  43c778: 83 c4 24                     	addl	$0x24, %esp
  43c77b: c3                           	retl
  43c77c: cc                           	int3
  43c77d: cc                           	int3
  43c77e: cc                           	int3
  43c77f: cc                           	int3
  43c780: 83 ec 20                     	subl	$0x20, %esp
  43c783: 56                           	pushl	%esi
  43c784: 6a 0c                        	pushl	$0xc
  43c786: e8 e6 72 02 00               	calll	0x463a71 <.text+0x62a71>
  43c78b: 33 c9                        	xorl	%ecx, %ecx
  43c78d: 83 c4 04                     	addl	$0x4, %esp
  43c790: 3b c1                        	cmpl	%ecx, %eax
  43c792: 74 0c                        	je	0x43c7a0 <.text+0x3b7a0>
  43c794: 89 08                        	movl	%ecx, (%eax)
  43c796: 89 48 04                     	movl	%ecx, 0x4(%eax)
  43c799: 89 48 08                     	movl	%ecx, 0x8(%eax)
  43c79c: 8b f0                        	movl	%eax, %esi
  43c79e: eb 02                        	jmp	0x43c7a2 <.text+0x3b7a2>
  43c7a0: 33 f6                        	xorl	%esi, %esi
  43c7a2: 89 4e 04                     	movl	%ecx, 0x4(%esi)
  43c7a5: 8d 44 24 08                  	leal	0x8(%esp), %eax
  43c7a9: 50                           	pushl	%eax
  43c7aa: 8d 4c 24 10                  	leal	0x10(%esp), %ecx
  43c7ae: 51                           	pushl	%ecx
  43c7af: 8d 54 24 18                  	leal	0x18(%esp), %edx
  43c7b3: 52                           	pushl	%edx
  43c7b4: 8d 44 24 20                  	leal	0x20(%esp), %eax
  43c7b8: 50                           	pushl	%eax
  43c7b9: 8d 4c 24 28                  	leal	0x28(%esp), %ecx
  43c7bd: 51                           	pushl	%ecx
  43c7be: 8d 54 24 30                  	leal	0x30(%esp), %edx
  43c7c2: 52                           	pushl	%edx
  43c7c3: 8d 44 24 38                  	leal	0x38(%esp), %eax
  43c7c7: 50                           	pushl	%eax
  43c7c8: 8d 4c 24 20                  	leal	0x20(%esp), %ecx
  43c7cc: 51                           	pushl	%ecx
  43c7cd: c7 06 01 00 00 00            	movl	$0x1, (%esi)
  43c7d3: e8 08 d4 fe ff               	calll	0x429be0 <.text+0x28be0>
  43c7d8: 83 c4 20                     	addl	$0x20, %esp
  43c7db: 3c 01                        	cmpb	$0x1, %al
  43c7dd: 8b c6                        	movl	%esi, %eax
  43c7df: 75 07                        	jne	0x43c7e8 <.text+0x3b7e8>
  43c7e1: 8b 54 24 04                  	movl	0x4(%esp), %edx
  43c7e5: 89 56 04                     	movl	%edx, 0x4(%esi)
  43c7e8: 5e                           	popl	%esi
  43c7e9: 83 c4 20                     	addl	$0x20, %esp
  43c7ec: c3                           	retl
  43c7ed: cc                           	int3
  43c7ee: cc                           	int3
  43c7ef: cc                           	int3
  43c7f0: 83 ec 20                     	subl	$0x20, %esp
  43c7f3: 56                           	pushl	%esi
  43c7f4: 6a 0c                        	pushl	$0xc
  43c7f6: e8 76 72 02 00               	calll	0x463a71 <.text+0x62a71>
  43c7fb: 33 c9                        	xorl	%ecx, %ecx
  43c7fd: 83 c4 04                     	addl	$0x4, %esp
  43c800: 3b c1                        	cmpl	%ecx, %eax
  43c802: 74 0c                        	je	0x43c810 <.text+0x3b810>
  43c804: 89 08                        	movl	%ecx, (%eax)
  43c806: 89 48 04                     	movl	%ecx, 0x4(%eax)
  43c809: 89 48 08                     	movl	%ecx, 0x8(%eax)
  43c80c: 8b f0                        	movl	%eax, %esi
  43c80e: eb 02                        	jmp	0x43c812 <.text+0x3b812>
  43c810: 33 f6                        	xorl	%esi, %esi
  43c812: 89 4e 04                     	movl	%ecx, 0x4(%esi)
  43c815: 8d 44 24 08                  	leal	0x8(%esp), %eax
  43c819: 50                           	pushl	%eax
  43c81a: 8d 4c 24 10                  	leal	0x10(%esp), %ecx
  43c81e: 51                           	pushl	%ecx
  43c81f: 8d 54 24 18                  	leal	0x18(%esp), %edx
  43c823: 52                           	pushl	%edx
  43c824: 8d 44 24 20                  	leal	0x20(%esp), %eax
  43c828: 50                           	pushl	%eax
  43c829: 8d 4c 24 28                  	leal	0x28(%esp), %ecx
  43c82d: 51                           	pushl	%ecx
  43c82e: 8d 54 24 30                  	leal	0x30(%esp), %edx
  43c832: 52                           	pushl	%edx
  43c833: 8d 44 24 1c                  	leal	0x1c(%esp), %eax
  43c837: 50                           	pushl	%eax
  43c838: 8d 4c 24 3c                  	leal	0x3c(%esp), %ecx
  43c83c: 51                           	pushl	%ecx
  43c83d: c7 06 01 00 00 00            	movl	$0x1, (%esi)
  43c843: e8 98 d3 fe ff               	calll	0x429be0 <.text+0x28be0>
  43c848: 83 c4 20                     	addl	$0x20, %esp
  43c84b: 3c 01                        	cmpb	$0x1, %al
  43c84d: 8b c6                        	movl	%esi, %eax
  43c84f: 75 07                        	jne	0x43c858 <.text+0x3b858>
  43c851: 8b 54 24 04                  	movl	0x4(%esp), %edx
  43c855: 89 56 04                     	movl	%edx, 0x4(%esi)
  43c858: 5e                           	popl	%esi
  43c859: 83 c4 20                     	addl	$0x20, %esp
  43c85c: c3                           	retl
  43c85d: cc                           	int3
  43c85e: cc                           	int3
  43c85f: cc                           	int3
  43c860: 83 ec 20                     	subl	$0x20, %esp
  43c863: 56                           	pushl	%esi
  43c864: 6a 0c                        	pushl	$0xc
  43c866: e8 06 72 02 00               	calll	0x463a71 <.text+0x62a71>
  43c86b: 33 c9                        	xorl	%ecx, %ecx
  43c86d: 83 c4 04                     	addl	$0x4, %esp
  43c870: 3b c1                        	cmpl	%ecx, %eax
  43c872: 74 0c                        	je	0x43c880 <.text+0x3b880>
  43c874: 89 08                        	movl	%ecx, (%eax)
  43c876: 89 48 04                     	movl	%ecx, 0x4(%eax)
  43c879: 89 48 08                     	movl	%ecx, 0x8(%eax)
  43c87c: 8b f0                        	movl	%eax, %esi
  43c87e: eb 02                        	jmp	0x43c882 <.text+0x3b882>
  43c880: 33 f6                        	xorl	%esi, %esi
  43c882: 89 4e 04                     	movl	%ecx, 0x4(%esi)
  43c885: 8d 44 24 08                  	leal	0x8(%esp), %eax
  43c889: 50                           	pushl	%eax
  43c88a: 8d 4c 24 10                  	leal	0x10(%esp), %ecx
  43c88e: 51                           	pushl	%ecx
  43c88f: 8d 54 24 18                  	leal	0x18(%esp), %edx
  43c893: 52                           	pushl	%edx
  43c894: 8d 44 24 20                  	leal	0x20(%esp), %eax
  43c898: 50                           	pushl	%eax
  43c899: 8d 4c 24 28                  	leal	0x28(%esp), %ecx
  43c89d: 51                           	pushl	%ecx
  43c89e: 8d 54 24 18                  	leal	0x18(%esp), %edx
  43c8a2: 52                           	pushl	%edx
  43c8a3: 8d 44 24 34                  	leal	0x34(%esp), %eax
  43c8a7: 50                           	pushl	%eax
  43c8a8: 8d 4c 24 3c                  	leal	0x3c(%esp), %ecx
  43c8ac: 51                           	pushl	%ecx
  43c8ad: c7 06 01 00 00 00            	movl	$0x1, (%esi)
  43c8b3: e8 28 d3 fe ff               	calll	0x429be0 <.text+0x28be0>
  43c8b8: 83 c4 20                     	addl	$0x20, %esp
  43c8bb: 3c 01                        	cmpb	$0x1, %al
  43c8bd: 8b c6                        	movl	%esi, %eax
  43c8bf: 75 07                        	jne	0x43c8c8 <.text+0x3b8c8>
  43c8c1: 8b 54 24 04                  	movl	0x4(%esp), %edx
  43c8c5: 89 56 04                     	movl	%edx, 0x4(%esi)
  43c8c8: 5e                           	popl	%esi
  43c8c9: 83 c4 20                     	addl	$0x20, %esp
  43c8cc: c3                           	retl
  43c8cd: cc                           	int3
  43c8ce: cc                           	int3
  43c8cf: cc                           	int3
  43c8d0: 83 ec 20                     	subl	$0x20, %esp
  43c8d3: 56                           	pushl	%esi
  43c8d4: 6a 0c                        	pushl	$0xc
  43c8d6: e8 96 71 02 00               	calll	0x463a71 <.text+0x62a71>
  43c8db: 33 c9                        	xorl	%ecx, %ecx
  43c8dd: 83 c4 04                     	addl	$0x4, %esp
  43c8e0: 3b c1                        	cmpl	%ecx, %eax
  43c8e2: 74 0c                        	je	0x43c8f0 <.text+0x3b8f0>
  43c8e4: 89 08                        	movl	%ecx, (%eax)
  43c8e6: 89 48 04                     	movl	%ecx, 0x4(%eax)
  43c8e9: 89 48 08                     	movl	%ecx, 0x8(%eax)
  43c8ec: 8b f0                        	movl	%eax, %esi
  43c8ee: eb 02                        	jmp	0x43c8f2 <.text+0x3b8f2>
  43c8f0: 33 f6                        	xorl	%esi, %esi
  43c8f2: 89 4e 04                     	movl	%ecx, 0x4(%esi)
  43c8f5: 8d 44 24 08                  	leal	0x8(%esp), %eax
  43c8f9: 50                           	pushl	%eax
  43c8fa: 8d 4c 24 10                  	leal	0x10(%esp), %ecx
  43c8fe: 51                           	pushl	%ecx
  43c8ff: 8d 54 24 18                  	leal	0x18(%esp), %edx
  43c903: 52                           	pushl	%edx
  43c904: 8d 44 24 20                  	leal	0x20(%esp), %eax
  43c908: 50                           	pushl	%eax
  43c909: 8d 4c 24 14                  	leal	0x14(%esp), %ecx
  43c90d: 51                           	pushl	%ecx
  43c90e: 8d 54 24 2c                  	leal	0x2c(%esp), %edx
  43c912: 52                           	pushl	%edx
  43c913: 8d 44 24 34                  	leal	0x34(%esp), %eax
  43c917: 50                           	pushl	%eax
  43c918: 8d 4c 24 3c                  	leal	0x3c(%esp), %ecx
  43c91c: 51                           	pushl	%ecx
  43c91d: c7 06 01 00 00 00            	movl	$0x1, (%esi)
  43c923: e8 b8 d2 fe ff               	calll	0x429be0 <.text+0x28be0>
  43c928: 83 c4 20                     	addl	$0x20, %esp
  43c92b: 3c 01                        	cmpb	$0x1, %al
  43c92d: 8b c6                        	movl	%esi, %eax
  43c92f: 75 07                        	jne	0x43c938 <.text+0x3b938>
  43c931: 8b 54 24 04                  	movl	0x4(%esp), %edx
  43c935: 89 56 04                     	movl	%edx, 0x4(%esi)
  43c938: 5e                           	popl	%esi
  43c939: 83 c4 20                     	addl	$0x20, %esp
  43c93c: c3                           	retl
  43c93d: cc                           	int3
  43c93e: cc                           	int3
  43c93f: cc                           	int3
  43c940: 83 ec 20                     	subl	$0x20, %esp
  43c943: 56                           	pushl	%esi
  43c944: 6a 0c                        	pushl	$0xc
  43c946: e8 26 71 02 00               	calll	0x463a71 <.text+0x62a71>
  43c94b: 33 c9                        	xorl	%ecx, %ecx
  43c94d: 83 c4 04                     	addl	$0x4, %esp
  43c950: 3b c1                        	cmpl	%ecx, %eax
  43c952: 74 0c                        	je	0x43c960 <.text+0x3b960>
  43c954: 89 08                        	movl	%ecx, (%eax)
  43c956: 89 48 04                     	movl	%ecx, 0x4(%eax)
  43c959: 89 48 08                     	movl	%ecx, 0x8(%eax)
  43c95c: 8b f0                        	movl	%eax, %esi
  43c95e: eb 02                        	jmp	0x43c962 <.text+0x3b962>
  43c960: 33 f6                        	xorl	%esi, %esi
  43c962: 89 4e 04                     	movl	%ecx, 0x4(%esi)
  43c965: 8d 44 24 08                  	leal	0x8(%esp), %eax
  43c969: 50                           	pushl	%eax
  43c96a: 8d 4c 24 10                  	leal	0x10(%esp), %ecx
  43c96e: 51                           	pushl	%ecx
  43c96f: 8d 54 24 18                  	leal	0x18(%esp), %edx
  43c973: 52                           	pushl	%edx
  43c974: 8d 44 24 10                  	leal	0x10(%esp), %eax
  43c978: 50                           	pushl	%eax
  43c979: 8d 4c 24 24                  	leal	0x24(%esp), %ecx
  43c97d: 51                           	pushl	%ecx
  43c97e: 8d 54 24 2c                  	leal	0x2c(%esp), %edx
  43c982: 52                           	pushl	%edx
  43c983: 8d 44 24 34                  	leal	0x34(%esp), %eax
  43c987: 50                           	pushl	%eax
  43c988: 8d 4c 24 3c                  	leal	0x3c(%esp), %ecx
  43c98c: 51                           	pushl	%ecx
  43c98d: c7 06 01 00 00 00            	movl	$0x1, (%esi)
  43c993: e8 48 d2 fe ff               	calll	0x429be0 <.text+0x28be0>
  43c998: 83 c4 20                     	addl	$0x20, %esp
  43c99b: 3c 01                        	cmpb	$0x1, %al
  43c99d: 8b c6                        	movl	%esi, %eax
  43c99f: 75 07                        	jne	0x43c9a8 <.text+0x3b9a8>
  43c9a1: 8b 54 24 04                  	movl	0x4(%esp), %edx
  43c9a5: 89 56 04                     	movl	%edx, 0x4(%esi)
  43c9a8: 5e                           	popl	%esi
  43c9a9: 83 c4 20                     	addl	$0x20, %esp
  43c9ac: c3                           	retl
  43c9ad: cc                           	int3
  43c9ae: cc                           	int3
  43c9af: cc                           	int3
  43c9b0: 83 ec 20                     	subl	$0x20, %esp
  43c9b3: 56                           	pushl	%esi
  43c9b4: 6a 0c                        	pushl	$0xc
  43c9b6: e8 b6 70 02 00               	calll	0x463a71 <.text+0x62a71>
  43c9bb: 33 c9                        	xorl	%ecx, %ecx
  43c9bd: 83 c4 04                     	addl	$0x4, %esp
  43c9c0: 3b c1                        	cmpl	%ecx, %eax
  43c9c2: 74 0c                        	je	0x43c9d0 <.text+0x3b9d0>
  43c9c4: 89 08                        	movl	%ecx, (%eax)
  43c9c6: 89 48 04                     	movl	%ecx, 0x4(%eax)
  43c9c9: 89 48 08                     	movl	%ecx, 0x8(%eax)
  43c9cc: 8b f0                        	movl	%eax, %esi
  43c9ce: eb 02                        	jmp	0x43c9d2 <.text+0x3b9d2>
  43c9d0: 33 f6                        	xorl	%esi, %esi
  43c9d2: 89 4e 04                     	movl	%ecx, 0x4(%esi)
  43c9d5: 8d 44 24 08                  	leal	0x8(%esp), %eax
  43c9d9: 50                           	pushl	%eax
  43c9da: 8d 4c 24 10                  	leal	0x10(%esp), %ecx
  43c9de: 51                           	pushl	%ecx
  43c9df: 8d 54 24 0c                  	leal	0xc(%esp), %edx
  43c9e3: 52                           	pushl	%edx
  43c9e4: 8d 44 24 1c                  	leal	0x1c(%esp), %eax
  43c9e8: 50                           	pushl	%eax
  43c9e9: 8d 4c 24 24                  	leal	0x24(%esp), %ecx
  43c9ed: 51                           	pushl	%ecx
  43c9ee: 8d 54 24 2c                  	leal	0x2c(%esp), %edx
  43c9f2: 52                           	pushl	%edx
  43c9f3: 8d 44 24 34                  	leal	0x34(%esp), %eax
  43c9f7: 50                           	pushl	%eax
  43c9f8: 8d 4c 24 3c                  	leal	0x3c(%esp), %ecx
  43c9fc: 51                           	pushl	%ecx
  43c9fd: c7 06 01 00 00 00            	movl	$0x1, (%esi)
  43ca03: e8 d8 d1 fe ff               	calll	0x429be0 <.text+0x28be0>
  43ca08: 83 c4 20                     	addl	$0x20, %esp
  43ca0b: 3c 01                        	cmpb	$0x1, %al
  43ca0d: 8b c6                        	movl	%esi, %eax
  43ca0f: 75 07                        	jne	0x43ca18 <.text+0x3ba18>
  43ca11: 8b 54 24 04                  	movl	0x4(%esp), %edx
  43ca15: 89 56 04                     	movl	%edx, 0x4(%esi)
  43ca18: 5e                           	popl	%esi
  43ca19: 83 c4 20                     	addl	$0x20, %esp
  43ca1c: c3                           	retl
  43ca1d: cc                           	int3
  43ca1e: cc                           	int3
  43ca1f: cc                           	int3
  43ca20: 83 ec 20                     	subl	$0x20, %esp
  43ca23: 56                           	pushl	%esi
  43ca24: 6a 0c                        	pushl	$0xc
  43ca26: e8 46 70 02 00               	calll	0x463a71 <.text+0x62a71>
  43ca2b: 33 c9                        	xorl	%ecx, %ecx
  43ca2d: 83 c4 04                     	addl	$0x4, %esp
  43ca30: 3b c1                        	cmpl	%ecx, %eax
  43ca32: 74 0c                        	je	0x43ca40 <.text+0x3ba40>
  43ca34: 89 08                        	movl	%ecx, (%eax)
  43ca36: 89 48 04                     	movl	%ecx, 0x4(%eax)
  43ca39: 89 48 08                     	movl	%ecx, 0x8(%eax)
  43ca3c: 8b f0                        	movl	%eax, %esi
  43ca3e: eb 02                        	jmp	0x43ca42 <.text+0x3ba42>
  43ca40: 33 f6                        	xorl	%esi, %esi
  43ca42: 89 4e 04                     	movl	%ecx, 0x4(%esi)
  43ca45: 8d 44 24 08                  	leal	0x8(%esp), %eax
  43ca49: 50                           	pushl	%eax
  43ca4a: 8d 4c 24 08                  	leal	0x8(%esp), %ecx
  43ca4e: 51                           	pushl	%ecx
  43ca4f: 8d 54 24 14                  	leal	0x14(%esp), %edx
  43ca53: 52                           	pushl	%edx
  43ca54: 8d 44 24 1c                  	leal	0x1c(%esp), %eax
  43ca58: 50                           	pushl	%eax
  43ca59: 8d 4c 24 24                  	leal	0x24(%esp), %ecx
  43ca5d: 51                           	pushl	%ecx
  43ca5e: 8d 54 24 2c                  	leal	0x2c(%esp), %edx
  43ca62: 52                           	pushl	%edx
  43ca63: 8d 44 24 34                  	leal	0x34(%esp), %eax
  43ca67: 50                           	pushl	%eax
  43ca68: 8d 4c 24 3c                  	leal	0x3c(%esp), %ecx
  43ca6c: 51                           	pushl	%ecx
  43ca6d: c7 06 01 00 00 00            	movl	$0x1, (%esi)
  43ca73: e8 68 d1 fe ff               	calll	0x429be0 <.text+0x28be0>
  43ca78: 83 c4 20                     	addl	$0x20, %esp
  43ca7b: 3c 01                        	cmpb	$0x1, %al
  43ca7d: 8b c6                        	movl	%esi, %eax
  43ca7f: 75 07                        	jne	0x43ca88 <.text+0x3ba88>
  43ca81: 8b 54 24 04                  	movl	0x4(%esp), %edx
  43ca85: 89 56 04                     	movl	%edx, 0x4(%esi)
  43ca88: 5e                           	popl	%esi
  43ca89: 83 c4 20                     	addl	$0x20, %esp
  43ca8c: c3                           	retl
  43ca8d: cc                           	int3
  43ca8e: cc                           	int3
  43ca8f: cc                           	int3
  43ca90: 83 ec 20                     	subl	$0x20, %esp
  43ca93: 56                           	pushl	%esi
  43ca94: 6a 0c                        	pushl	$0xc
  43ca96: e8 d6 6f 02 00               	calll	0x463a71 <.text+0x62a71>
  43ca9b: 33 c9                        	xorl	%ecx, %ecx
  43ca9d: 83 c4 04                     	addl	$0x4, %esp
  43caa0: 3b c1                        	cmpl	%ecx, %eax
  43caa2: 74 0c                        	je	0x43cab0 <.text+0x3bab0>
  43caa4: 89 08                        	movl	%ecx, (%eax)
  43caa6: 89 48 04                     	movl	%ecx, 0x4(%eax)
  43caa9: 89 48 08                     	movl	%ecx, 0x8(%eax)
  43caac: 8b f0                        	movl	%eax, %esi
  43caae: eb 02                        	jmp	0x43cab2 <.text+0x3bab2>
  43cab0: 33 f6                        	xorl	%esi, %esi
  43cab2: 89 4e 04                     	movl	%ecx, 0x4(%esi)
  43cab5: 8d 44 24 04                  	leal	0x4(%esp), %eax
  43cab9: 50                           	pushl	%eax
  43caba: 8d 4c 24 0c                  	leal	0xc(%esp), %ecx
  43cabe: 51                           	pushl	%ecx
  43cabf: 8d 54 24 14                  	leal	0x14(%esp), %edx
  43cac3: 52                           	pushl	%edx
  43cac4: 8d 44 24 1c                  	leal	0x1c(%esp), %eax
  43cac8: 50                           	pushl	%eax
  43cac9: 8d 4c 24 24                  	leal	0x24(%esp), %ecx
  43cacd: 51                           	pushl	%ecx
  43cace: 8d 54 24 2c                  	leal	0x2c(%esp), %edx
  43cad2: 52                           	pushl	%edx
  43cad3: 8d 44 24 34                  	leal	0x34(%esp), %eax
  43cad7: 50                           	pushl	%eax
  43cad8: 8d 4c 24 3c                  	leal	0x3c(%esp), %ecx
  43cadc: 51                           	pushl	%ecx
  43cadd: c7 06 01 00 00 00            	movl	$0x1, (%esi)
  43cae3: e8 f8 d0 fe ff               	calll	0x429be0 <.text+0x28be0>
  43cae8: 83 c4 20                     	addl	$0x20, %esp
  43caeb: 3c 01                        	cmpb	$0x1, %al
  43caed: 8b c6                        	movl	%esi, %eax
  43caef: 75 07                        	jne	0x43caf8 <.text+0x3baf8>
  43caf1: 8b 54 24 04                  	movl	0x4(%esp), %edx
  43caf5: 89 56 04                     	movl	%edx, 0x4(%esi)
  43caf8: 5e                           	popl	%esi
  43caf9: 83 c4 20                     	addl	$0x20, %esp
  43cafc: c3                           	retl
  43cafd: cc                           	int3
  43cafe: cc                           	int3
  43caff: cc                           	int3
  43cb00: 56                           	pushl	%esi
  43cb01: 6a 0c                        	pushl	$0xc
  43cb03: e8 69 6f 02 00               	calll	0x463a71 <.text+0x62a71>
  43cb08: 33 c9                        	xorl	%ecx, %ecx
  43cb0a: 83 c4 04                     	addl	$0x4, %esp
  43cb0d: 3b c1                        	cmpl	%ecx, %eax
  43cb0f: 74 0c                        	je	0x43cb1d <.text+0x3bb1d>
  43cb11: 89 08                        	movl	%ecx, (%eax)
  43cb13: 89 48 04                     	movl	%ecx, 0x4(%eax)
  43cb16: 89 48 08                     	movl	%ecx, 0x8(%eax)
  43cb19: 8b f0                        	movl	%eax, %esi
  43cb1b: eb 02                        	jmp	0x43cb1f <.text+0x3bb1f>
  43cb1d: 33 f6                        	xorl	%esi, %esi
  43cb1f: 89 4e 04                     	movl	%ecx, 0x4(%esi)
  43cb22: c7 06 01 00 00 00            	movl	$0x1, (%esi)
  43cb28: a1 dc c8 56 00               	movl	0x56c8dc, %eax
  43cb2d: 89 46 04                     	movl	%eax, 0x4(%esi)
  43cb30: ff 15 a0 33 4a 00            	calll	*0x4a33a0
  43cb36: 8b c8                        	movl	%eax, %ecx
  43cb38: 2b 0d e0 c8 56 00            	subl	0x56c8e0, %ecx
  43cb3e: b8 d3 4d 62 10               	movl	$0x10624dd3, %eax       # imm = 0x10624DD3
  43cb43: f7 e1                        	mull	%ecx
  43cb45: c1 ea 06                     	shrl	$0x6, %edx
  43cb48: 01 56 04                     	addl	%edx, 0x4(%esi)
  43cb4b: 8b c6                        	movl	%esi, %eax
  43cb4d: 5e                           	popl	%esi
  43cb4e: c3                           	retl
  43cb4f: cc                           	int3
  43cb50: c7 05 dc c8 56 00 00 00 00 00	movl	$0x0, 0x56c8dc
  43cb5a: ff 15 a0 33 4a 00            	calll	*0x4a33a0
  43cb60: a3 e0 c8 56 00               	movl	%eax, 0x56c8e0
  43cb65: 33 c0                        	xorl	%eax, %eax
  43cb67: c3                           	retl
  43cb68: cc                           	int3
  43cb69: cc                           	int3
  43cb6a: cc                           	int3
  43cb6b: cc                           	int3
  43cb6c: cc                           	int3
  43cb6d: cc                           	int3
  43cb6e: cc                           	int3
  43cb6f: cc                           	int3
  43cb70: 33 c0                        	xorl	%eax, %eax
  43cb72: 83 7c 24 08 01               	cmpl	$0x1, 0x8(%esp)
  43cb77: 7c 5a                        	jl	0x43cbd3 <.text+0x3bbd3>
  43cb79: 8b 44 24 0c                  	movl	0xc(%esp), %eax
  43cb7d: 56                           	pushl	%esi
  43cb7e: 8b 30                        	movl	(%eax), %esi
  43cb80: 57                           	pushl	%edi
  43cb81: 6a 20                        	pushl	$0x20
  43cb83: e8 79 77 02 00               	calll	0x464301 <.text+0x63301>
  43cb88: 8b 0e                        	movl	(%esi), %ecx
  43cb8a: 51                           	pushl	%ecx
  43cb8b: 8b f8                        	movl	%eax, %edi
  43cb8d: 68 7c 66 4a 00               	pushl	$0x4a667c               # imm = 0x4A667C
  43cb92: 57                           	pushl	%edi
  43cb93: e8 8d 6f 02 00               	calll	0x463b25 <.text+0x62b25>
  43cb98: 6a 0c                        	pushl	$0xc
  43cb9a: e8 d2 6e 02 00               	calll	0x463a71 <.text+0x62a71>
  43cb9f: 83 c4 14                     	addl	$0x14, %esp
  43cba2: 85 c0                        	testl	%eax, %eax
  43cba4: 74 20                        	je	0x43cbc6 <.text+0x3bbc6>
  43cba6: c7 00 00 00 00 00            	movl	$0x0, (%eax)
  43cbac: c7 40 04 00 00 00 00         	movl	$0x0, 0x4(%eax)
  43cbb3: c7 40 08 00 00 00 00         	movl	$0x0, 0x8(%eax)
  43cbba: 89 78 08                     	movl	%edi, 0x8(%eax)
  43cbbd: 5f                           	popl	%edi
  43cbbe: c7 00 02 00 00 00            	movl	$0x2, (%eax)
  43cbc4: 5e                           	popl	%esi
  43cbc5: c3                           	retl
  43cbc6: 33 c0                        	xorl	%eax, %eax
  43cbc8: 89 78 08                     	movl	%edi, 0x8(%eax)
  43cbcb: 5f                           	popl	%edi
  43cbcc: c7 00 02 00 00 00            	movl	$0x2, (%eax)
  43cbd2: 5e                           	popl	%esi
  43cbd3: c3                           	retl


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  429be0: 8d 44 24 f0                  	leal	-0x10(%esp), %eax
  429be4: 83 ec 10                     	subl	$0x10, %esp
  429be7: 50                           	pushl	%eax
  429be8: e8 89 be 03 00               	calll	0x465a76 <.text+0x64a76>
  429bed: 8d 4c 24 04                  	leal	0x4(%esp), %ecx
  429bf1: 51                           	pushl	%ecx
  429bf2: e8 09 bd 03 00               	calll	0x465900 <.text+0x64900>
  429bf7: 83 c4 08                     	addl	$0x8, %esp
  429bfa: 85 c0                        	testl	%eax, %eax
  429bfc: 74 5c                        	je	0x429c5a <.text+0x28c5a>
  429bfe: 8b 50 14                     	movl	0x14(%eax), %edx
  429c01: 56                           	pushl	%esi
  429c02: 8b 74 24 18                  	movl	0x18(%esp), %esi
  429c06: 81 c2 6c 07 00 00            	addl	$0x76c, %edx            # imm = 0x76C
  429c0c: 89 16                        	movl	%edx, (%esi)
  429c0e: 8b 50 10                     	movl	0x10(%eax), %edx
  429c11: 8b 74 24 1c                  	movl	0x1c(%esp), %esi
  429c15: 83 c2 01                     	addl	$0x1, %edx
  429c18: 89 16                        	movl	%edx, (%esi)
  429c1a: 8b 50 0c                     	movl	0xc(%eax), %edx
  429c1d: 8b 74 24 20                  	movl	0x20(%esp), %esi
  429c21: 89 16                        	movl	%edx, (%esi)
  429c23: 8b 50 08                     	movl	0x8(%eax), %edx
  429c26: 8b 74 24 24                  	movl	0x24(%esp), %esi
  429c2a: 89 16                        	movl	%edx, (%esi)
  429c2c: 8b 50 04                     	movl	0x4(%eax), %edx
  429c2f: 8b 74 24 28                  	movl	0x28(%esp), %esi
  429c33: 89 16                        	movl	%edx, (%esi)
  429c35: 8b 10                        	movl	(%eax), %edx
  429c37: 8b 74 24 2c                  	movl	0x2c(%esp), %esi
  429c3b: 89 16                        	movl	%edx, (%esi)
  429c3d: 0f b7 54 24 0c               	movzwl	0xc(%esp), %edx
  429c42: 8b 74 24 30                  	movl	0x30(%esp), %esi
  429c46: 89 16                        	movl	%edx, (%esi)
  429c48: 8b 40 18                     	movl	0x18(%eax), %eax
  429c4b: 8b 54 24 34                  	movl	0x34(%esp), %edx
  429c4f: b1 01                        	movb	$0x1, %cl
  429c51: 89 02                        	movl	%eax, (%edx)
  429c53: 8a c1                        	movb	%cl, %al
  429c55: 5e                           	popl	%esi
  429c56: 83 c4 10                     	addl	$0x10, %esp
  429c59: c3                           	retl
  429c5a: 32 c0                        	xorb	%al, %al
  429c5c: 83 c4 10                     	addl	$0x10, %esp
  429c5f: c3                           	retl


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  429930: 81 ec 44 01 00 00            	subl	$0x144, %esp            # imm = 0x144
  429936: a1 48 ea 4b 00               	movl	0x4bea48, %eax
  42993b: 33 c4                        	xorl	%esp, %eax
  42993d: 89 84 24 40 01 00 00         	movl	%eax, 0x140(%esp)
  429944: 8b 84 24 48 01 00 00         	movl	0x148(%esp), %eax
  42994b: 53                           	pushl	%ebx
  42994c: 56                           	pushl	%esi
  42994d: 57                           	pushl	%edi
  42994e: 8d 4c 24 0c                  	leal	0xc(%esp), %ecx
  429952: 51                           	pushl	%ecx
  429953: 50                           	pushl	%eax
  429954: 32 db                        	xorb	%bl, %bl
  429956: be 01 00 00 00               	movl	$0x1, %esi
  42995b: ff 15 d4 30 4a 00            	calll	*0x4a30d4
  429961: 8b f8                        	movl	%eax, %edi
  429963: 83 ff ff                     	cmpl	$-0x1, %edi
  429966: 74 22                        	je	0x42998a <.text+0x2898a>
  429968: 55                           	pushl	%ebp
  429969: 8b 2d d0 30 4a 00            	movl	0x4a30d0, %ebp
  42996f: 90                           	nop
  429970: 85 f6                        	testl	%esi, %esi
  429972: 74 15                        	je	0x429989 <.text+0x28989>
  429974: f6 44 24 10 10               	testb	$0x10, 0x10(%esp)
  429979: 74 0c                        	je	0x429987 <.text+0x28987>
  42997b: 8d 54 24 10                  	leal	0x10(%esp), %edx
  42997f: 52                           	pushl	%edx
  429980: 57                           	pushl	%edi
  429981: ff d5                        	calll	*%ebp
  429983: 8b f0                        	movl	%eax, %esi
  429985: eb e9                        	jmp	0x429970 <.text+0x28970>
  429987: b3 01                        	movb	$0x1, %bl
  429989: 5d                           	popl	%ebp
  42998a: 57                           	pushl	%edi
  42998b: ff 15 cc 30 4a 00            	calll	*0x4a30cc
  429991: 8b 8c 24 4c 01 00 00         	movl	0x14c(%esp), %ecx
  429998: 5f                           	popl	%edi
  429999: 5e                           	popl	%esi
  42999a: 8a c3                        	movb	%bl, %al
  42999c: 5b                           	popl	%ebx
  42999d: 33 cc                        	xorl	%esp, %ecx
  42999f: e8 18 a2 03 00               	calll	0x463bbc <.text+0x62bbc>
  4299a4: 81 c4 44 01 00 00            	addl	$0x144, %esp            # imm = 0x144
  4299aa: c3                           	retl
  4299ab: cc                           	int3
  4299ac: cc                           	int3
  4299ad: cc                           	int3
  4299ae: cc                           	int3
  4299af: cc                           	int3


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  432600: 8b 44 24 08                  	movl	0x8(%esp), %eax
  432604: 8b 4c 24 04                  	movl	0x4(%esp), %ecx
  432608: 56                           	pushl	%esi
  432609: 57                           	pushl	%edi
  43260a: be 10 00 00 00               	movl	$0x10, %esi
  43260f: 90                           	nop
  432610: 8b 11                        	movl	(%ecx), %edx
  432612: 3b 10                        	cmpl	(%eax), %edx
  432614: 75 12                        	jne	0x432628 <.text+0x31628>
  432616: 83 ee 04                     	subl	$0x4, %esi
  432619: 83 c0 04                     	addl	$0x4, %eax
  43261c: 83 c1 04                     	addl	$0x4, %ecx
  43261f: 83 fe 04                     	cmpl	$0x4, %esi
  432622: 73 ec                        	jae	0x432610 <.text+0x31610>
  432624: 85 f6                        	testl	%esi, %esi
  432626: 74 67                        	je	0x43268f <.text+0x3168f>
  432628: 0f b6 11                     	movzbl	(%ecx), %edx
  43262b: 0f b6 38                     	movzbl	(%eax), %edi
  43262e: 2b d7                        	subl	%edi, %edx
  432630: 75 45                        	jne	0x432677 <.text+0x31677>
  432632: 83 ee 01                     	subl	$0x1, %esi
  432635: 83 c0 01                     	addl	$0x1, %eax
  432638: 83 c1 01                     	addl	$0x1, %ecx
  43263b: 85 f6                        	testl	%esi, %esi
  43263d: 74 50                        	je	0x43268f <.text+0x3168f>
  43263f: 0f b6 11                     	movzbl	(%ecx), %edx
  432642: 0f b6 38                     	movzbl	(%eax), %edi
  432645: 2b d7                        	subl	%edi, %edx
  432647: 75 2e                        	jne	0x432677 <.text+0x31677>
  432649: 83 ee 01                     	subl	$0x1, %esi
  43264c: 83 c0 01                     	addl	$0x1, %eax
  43264f: 83 c1 01                     	addl	$0x1, %ecx
  432652: 85 f6                        	testl	%esi, %esi
  432654: 74 39                        	je	0x43268f <.text+0x3168f>
  432656: 0f b6 11                     	movzbl	(%ecx), %edx
  432659: 0f b6 38                     	movzbl	(%eax), %edi
  43265c: 2b d7                        	subl	%edi, %edx
  43265e: 75 17                        	jne	0x432677 <.text+0x31677>
  432660: 83 ee 01                     	subl	$0x1, %esi
  432663: 83 c0 01                     	addl	$0x1, %eax
  432666: 83 c1 01                     	addl	$0x1, %ecx
  432669: 85 f6                        	testl	%esi, %esi
  43266b: 74 22                        	je	0x43268f <.text+0x3168f>
  43266d: 0f b6 11                     	movzbl	(%ecx), %edx
  432670: 0f b6 00                     	movzbl	(%eax), %eax
  432673: 2b d0                        	subl	%eax, %edx
  432675: 74 18                        	je	0x43268f <.text+0x3168f>
  432677: 85 d2                        	testl	%edx, %edx
  432679: b8 01 00 00 00               	movl	$0x1, %eax
  43267e: 7f 11                        	jg	0x432691 <.text+0x31691>
  432680: 83 c8 ff                     	orl	$-0x1, %eax
  432683: 33 c9                        	xorl	%ecx, %ecx
  432685: 85 c0                        	testl	%eax, %eax
  432687: 0f 94 c1                     	sete	%cl
  43268a: 5f                           	popl	%edi
  43268b: 5e                           	popl	%esi
  43268c: 8b c1                        	movl	%ecx, %eax
  43268e: c3                           	retl
  43268f: 33 c0                        	xorl	%eax, %eax
  432691: 33 c9                        	xorl	%ecx, %ecx
  432693: 85 c0                        	testl	%eax, %eax
  432695: 0f 94 c1                     	sete	%cl
  432698: 5f                           	popl	%edi
  432699: 5e                           	popl	%esi
  43269a: 8b c1                        	movl	%ecx, %eax
  43269c: c3                           	retl


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  464ca0: 83 3d 24 e9 57 00 00         	cmpl	$0x0, 0x57e924
  464ca7: 74 2d                        	je	0x464cd6 <.text+0x63cd6>
  464ca9: 55                           	pushl	%ebp
  464caa: 8b ec                        	movl	%esp, %ebp
  464cac: 83 ec 08                     	subl	$0x8, %esp
  464caf: 83 e4 f8                     	andl	$-0x8, %esp
  464cb2: dd 1c 24                     	fstpl	(%esp)
  464cb5: f2 0f 2c 04 24               	cvttsd2si	(%esp), %eax
  464cba: c9                           	leave
  464cbb: c3                           	retl
  464cbc: 83 3d 24 e9 57 00 00         	cmpl	$0x0, 0x57e924
  464cc3: 74 11                        	je	0x464cd6 <.text+0x63cd6>
  464cc5: 83 ec 04                     	subl	$0x4, %esp
  464cc8: d9 3c 24                     	fnstcw	(%esp)
  464ccb: 58                           	popl	%eax
  464ccc: 66 83 e0 7f                  	andw	$0x7f, %ax
  464cd0: 66 83 f8 7f                  	cmpw	$0x7f, %ax
  464cd4: 74 d3                        	je	0x464ca9 <.text+0x63ca9>
  464cd6: 55                           	pushl	%ebp
  464cd7: 8b ec                        	movl	%esp, %ebp
  464cd9: 83 ec 20                     	subl	$0x20, %esp
  464cdc: 83 e4 f0                     	andl	$-0x10, %esp
  464cdf: d9 c0                        	fld	%st(0)
  464ce1: d9 54 24 18                  	fsts	0x18(%esp)
  464ce5: df 7c 24 10                  	fistpll	0x10(%esp)
  464ce9: df 6c 24 10                  	fildll	0x10(%esp)
  464ced: 8b 54 24 18                  	movl	0x18(%esp), %edx
  464cf1: 8b 44 24 10                  	movl	0x10(%esp), %eax
  464cf5: 85 c0                        	testl	%eax, %eax
  464cf7: 74 3c                        	je	0x464d35 <.text+0x63d35>
  464cf9: de e9                        	fsubrp	%st, %st(1)
  464cfb: 85 d2                        	testl	%edx, %edx
  464cfd: 79 1e                        	jns	0x464d1d <.text+0x63d1d>
  464cff: d9 1c 24                     	fstps	(%esp)
  464d02: 8b 0c 24                     	movl	(%esp), %ecx
  464d05: 81 f1 00 00 00 80            	xorl	$0x80000000, %ecx       # imm = 0x80000000
  464d0b: 81 c1 ff ff ff 7f            	addl	$0x7fffffff, %ecx       # imm = 0x7FFFFFFF
  464d11: 83 d0 00                     	adcl	$0x0, %eax
  464d14: 8b 54 24 14                  	movl	0x14(%esp), %edx
  464d18: 83 d2 00                     	adcl	$0x0, %edx
  464d1b: eb 2c                        	jmp	0x464d49 <.text+0x63d49>
  464d1d: d9 1c 24                     	fstps	(%esp)
  464d20: 8b 0c 24                     	movl	(%esp), %ecx
  464d23: 81 c1 ff ff ff 7f            	addl	$0x7fffffff, %ecx       # imm = 0x7FFFFFFF
  464d29: 83 d8 00                     	sbbl	$0x0, %eax
  464d2c: 8b 54 24 14                  	movl	0x14(%esp), %edx
  464d30: 83 da 00                     	sbbl	$0x0, %edx
  464d33: eb 14                        	jmp	0x464d49 <.text+0x63d49>
  464d35: 8b 54 24 14                  	movl	0x14(%esp), %edx
  464d39: f7 c2 ff ff ff 7f            	testl	$0x7fffffff, %edx       # imm = 0x7FFFFFFF
  464d3f: 75 b8                        	jne	0x464cf9 <.text+0x63cf9>
  464d41: d9 5c 24 18                  	fstps	0x18(%esp)
  464d45: d9 5c 24 18                  	fstps	0x18(%esp)
  464d49: c9                           	leave
  464d4a: c3                           	retl
  464d4b: cc                           	int3
  464d4c: cc                           	int3
  464d4d: cc                           	int3
  464d4e: cc                           	int3
  464d4f: cc                           	int3
  464d50: 8b 4c 24 0c                  	movl	0xc(%esp), %ecx
  464d54: 57                           	pushl	%edi
  464d55: 85 c9                        	testl	%ecx, %ecx
  464d57: 0f 84 92 00 00 00            	je	0x464def <.text+0x63def>
  464d5d: 56                           	pushl	%esi
  464d5e: 53                           	pushl	%ebx
  464d5f: 8b d9                        	movl	%ecx, %ebx


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  465190: 83 3d 20 e9 57 00 00         	cmpl	$0x0, 0x57e920
  465197: 74 32                        	je	0x4651cb <.text+0x641cb>
  465199: 83 ec 08                     	subl	$0x8, %esp
  46519c: 0f ae 5c 24 04               	stmxcsr	0x4(%esp)
  4651a1: 8b 44 24 04                  	movl	0x4(%esp), %eax
  4651a5: 25 80 1f 00 00               	andl	$0x1f80, %eax           # imm = 0x1F80
  4651aa: 3d 80 1f 00 00               	cmpl	$0x1f80, %eax           # imm = 0x1F80
  4651af: 75 0f                        	jne	0x4651c0 <.text+0x641c0>
  4651b1: d9 3c 24                     	fnstcw	(%esp)
  4651b4: 66 8b 04 24                  	movw	(%esp), %ax
  4651b8: 66 83 e0 7f                  	andw	$0x7f, %ax
  4651bc: 66 83 f8 7f                  	cmpw	$0x7f, %ax
  4651c0: 8d 64 24 08                  	leal	0x8(%esp), %esp
  4651c4: 75 05                        	jne	0x4651cb <.text+0x641cb>
  4651c6: e9 25 79 00 00               	jmp	0x46caf0 <.text+0x6baf0>
  4651cb: 83 ec 0c                     	subl	$0xc, %esp
  4651ce: dd 14 24                     	fstl	(%esp)
  4651d1: e8 12 69 00 00               	calll	0x46bae8 <.text+0x6aae8>
  4651d6: e8 0d 00 00 00               	calll	0x4651e8 <.text+0x641e8>
  4651db: 83 c4 0c                     	addl	$0xc, %esp
  4651de: c3                           	retl
  4651df: 8d 54 24 04                  	leal	0x4(%esp), %edx
  4651e3: e8 bd 68 00 00               	calll	0x46baa5 <.text+0x6aaa5>
  4651e8: 52                           	pushl	%edx
  4651e9: 9b                           	wait
  4651ea: d9 3c 24                     	fnstcw	(%esp)
  4651ed: 74 50                        	je	0x46523f <.text+0x6423f>
  4651ef: 66 81 3c 24 7f 02            	cmpw	$0x27f, (%esp)          # imm = 0x27F
  4651f5: 74 06                        	je	0x4651fd <.text+0x641fd>
  4651f7: d9 2d 68 9e 4a 00            	fldcw	0x4a9e68
  4651fd: d9 ff                        	fcos
  4651ff: 9b                           	wait
  465200: df e0                        	fnstsw	%ax
  465202: 9e                           	sahf
  465203: 7a 1d                        	jp	0x465222 <.text+0x64222>
  465205: 83 3d e8 db 57 00 00         	cmpl	$0x0, 0x57dbe8
  46520c: 0f 85 ec 68 00 00            	jne	0x46bafe <.text+0x6aafe>


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  4652c0: 83 3d 20 e9 57 00 00         	cmpl	$0x0, 0x57e920
  4652c7: 74 32                        	je	0x4652fb <.text+0x642fb>
  4652c9: 83 ec 08                     	subl	$0x8, %esp
  4652cc: 0f ae 5c 24 04               	stmxcsr	0x4(%esp)
  4652d1: 8b 44 24 04                  	movl	0x4(%esp), %eax
  4652d5: 25 80 1f 00 00               	andl	$0x1f80, %eax           # imm = 0x1F80
  4652da: 3d 80 1f 00 00               	cmpl	$0x1f80, %eax           # imm = 0x1F80
  4652df: 75 0f                        	jne	0x4652f0 <.text+0x642f0>
  4652e1: d9 3c 24                     	fnstcw	(%esp)
  4652e4: 66 8b 04 24                  	movw	(%esp), %ax
  4652e8: 66 83 e0 7f                  	andw	$0x7f, %ax
  4652ec: 66 83 f8 7f                  	cmpw	$0x7f, %ax
  4652f0: 8d 64 24 08                  	leal	0x8(%esp), %esp
  4652f4: 75 05                        	jne	0x4652fb <.text+0x642fb>
  4652f6: e9 a5 79 00 00               	jmp	0x46cca0 <.text+0x6bca0>
  4652fb: 83 ec 0c                     	subl	$0xc, %esp
  4652fe: dd 14 24                     	fstl	(%esp)
  465301: e8 e2 67 00 00               	calll	0x46bae8 <.text+0x6aae8>
  465306: e8 0d 00 00 00               	calll	0x465318 <.text+0x64318>
  46530b: 83 c4 0c                     	addl	$0xc, %esp
  46530e: c3                           	retl
  46530f: 8d 54 24 04                  	leal	0x4(%esp), %edx
  465313: e8 8d 67 00 00               	calll	0x46baa5 <.text+0x6aaa5>
  465318: 52                           	pushl	%edx
  465319: 9b                           	wait
  46531a: d9 3c 24                     	fnstcw	(%esp)
  46531d: 74 50                        	je	0x46536f <.text+0x6436f>
  46531f: 66 81 3c 24 7f 02            	cmpw	$0x27f, (%esp)          # imm = 0x27F
  465325: 74 06                        	je	0x46532d <.text+0x6432d>
  465327: d9 2d 68 9e 4a 00            	fldcw	0x4a9e68
  46532d: d9 fe                        	fsin
  46532f: 9b                           	wait
  465330: df e0                        	fnstsw	%ax
  465332: 9e                           	sahf
  465333: 7a 1d                        	jp	0x465352 <.text+0x64352>
  465335: 83 3d e8 db 57 00 00         	cmpl	$0x0, 0x57dbe8
  46533c: 0f 85 bc 67 00 00            	jne	0x46bafe <.text+0x6aafe>


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  4660b0: 83 3d 20 e9 57 00 00         	cmpl	$0x0, 0x57e920
  4660b7: 74 32                        	je	0x4660eb <.text+0x650eb>
  4660b9: 83 ec 08                     	subl	$0x8, %esp
  4660bc: 0f ae 5c 24 04               	stmxcsr	0x4(%esp)
  4660c1: 8b 44 24 04                  	movl	0x4(%esp), %eax
  4660c5: 25 80 1f 00 00               	andl	$0x1f80, %eax           # imm = 0x1F80
  4660ca: 3d 80 1f 00 00               	cmpl	$0x1f80, %eax           # imm = 0x1F80
  4660cf: 75 0f                        	jne	0x4660e0 <.text+0x650e0>
  4660d1: d9 3c 24                     	fnstcw	(%esp)
  4660d4: 66 8b 04 24                  	movw	(%esp), %ax
  4660d8: 66 83 e0 7f                  	andw	$0x7f, %ax
  4660dc: 66 83 f8 7f                  	cmpw	$0x7f, %ax
  4660e0: 8d 64 24 08                  	leal	0x8(%esp), %esp
  4660e4: 75 05                        	jne	0x4660eb <.text+0x650eb>
  4660e6: e9 95 94 00 00               	jmp	0x46f580 <.text+0x6e580>
  4660eb: 83 ec 0c                     	subl	$0xc, %esp
  4660ee: dd 14 24                     	fstl	(%esp)
  4660f1: e8 f2 59 00 00               	calll	0x46bae8 <.text+0x6aae8>
  4660f6: e8 0d 00 00 00               	calll	0x466108 <.text+0x65108>
  4660fb: 83 c4 0c                     	addl	$0xc, %esp
  4660fe: c3                           	retl
  4660ff: 8d 54 24 04                  	leal	0x4(%esp), %edx
  466103: e8 9d 59 00 00               	calll	0x46baa5 <.text+0x6aaa5>
  466108: 52                           	pushl	%edx
  466109: 9b                           	wait
  46610a: d9 3c 24                     	fnstcw	(%esp)
  46610d: 74 54                        	je	0x466163 <.text+0x65163>
  46610f: 66 81 3c 24 7f 02            	cmpw	$0x27f, (%esp)          # imm = 0x27F
  466115: 74 06                        	je	0x46611d <.text+0x6511d>
  466117: d9 2d 68 9e 4a 00            	fldcw	0x4a9e68
  46611d: d9 f2                        	fptan
  46611f: 9b                           	wait
  466120: df e0                        	fnstsw	%ax
  466122: 9e                           	sahf
  466123: 7a 1f                        	jp	0x466144 <.text+0x65144>
  466125: dd d8                        	fstp	%st(0)
  466127: 83 3d e8 db 57 00 00         	cmpl	$0x0, 0x57dbe8
  46612e: 0f 85 ca 59 00 00            	jne	0x46bafe <.text+0x6aafe>


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  4326f0: 53                           	pushl	%ebx
  4326f1: 55                           	pushl	%ebp
  4326f2: 56                           	pushl	%esi
  4326f3: 57                           	pushl	%edi
  4326f4: 6a 40                        	pushl	$0x40
  4326f6: 8b f1                        	movl	%ecx, %esi
  4326f8: 6a 00                        	pushl	$0x0
  4326fa: 56                           	pushl	%esi
  4326fb: e8 10 1c 03 00               	calll	0x464310 <.text+0x63310>
  432700: 8b 54 24 20                  	movl	0x20(%esp), %edx
  432704: 8b 5c 24 24                  	movl	0x24(%esp), %ebx
  432708: bf 02 00 00 00               	movl	$0x2, %edi
  43270d: 83 c4 0c                     	addl	$0xc, %esp
  432710: c7 46 40 00 00 00 00         	movl	$0x0, 0x40(%esi)
  432717: 83 c2 08                     	addl	$0x8, %edx
  43271a: 8d 47 ff                     	leal	-0x1(%edi), %eax
  43271d: 8d 49 00                     	leal	(%ecx), %ecx
  432720: 8d 4f fe                     	leal	-0x2(%edi), %ecx
  432723: 3b cb                        	cmpl	%ebx, %ecx
  432725: 7d 5f                        	jge	0x432786 <.text+0x31786>
  432727: 8b 4a f8                     	movl	-0x8(%edx), %ecx
  43272a: 85 c9                        	testl	%ecx, %ecx
  43272c: 74 09                        	je	0x432737 <.text+0x31737>
  43272e: 8b 6e 40                     	movl	0x40(%esi), %ebp
  432731: 89 0c ae                     	movl	%ecx, (%esi,%ebp,4)
  432734: 01 46 40                     	addl	%eax, 0x40(%esi)
  432737: 8d 4f ff                     	leal	-0x1(%edi), %ecx
  43273a: 3b cb                        	cmpl	%ebx, %ecx
  43273c: 7d 48                        	jge	0x432786 <.text+0x31786>
  43273e: 8b 4a fc                     	movl	-0x4(%edx), %ecx
  432741: 85 c9                        	testl	%ecx, %ecx
  432743: 74 09                        	je	0x43274e <.text+0x3174e>
  432745: 8b 6e 40                     	movl	0x40(%esi), %ebp
  432748: 89 0c ae                     	movl	%ecx, (%esi,%ebp,4)
  43274b: 01 46 40                     	addl	%eax, 0x40(%esi)
  43274e: 3b fb                        	cmpl	%ebx, %edi
  432750: 7d 34                        	jge	0x432786 <.text+0x31786>
  432752: 8b 0a                        	movl	(%edx), %ecx
  432754: 85 c9                        	testl	%ecx, %ecx
  432756: 74 09                        	je	0x432761 <.text+0x31761>
  432758: 8b 6e 40                     	movl	0x40(%esi), %ebp
  43275b: 89 0c ae                     	movl	%ecx, (%esi,%ebp,4)
  43275e: 01 46 40                     	addl	%eax, 0x40(%esi)
  432761: 8d 4f 01                     	leal	0x1(%edi), %ecx
  432764: 3b cb                        	cmpl	%ebx, %ecx
  432766: 7d 1e                        	jge	0x432786 <.text+0x31786>
  432768: 8b 4a 04                     	movl	0x4(%edx), %ecx
  43276b: 85 c9                        	testl	%ecx, %ecx
  43276d: 74 09                        	je	0x432778 <.text+0x31778>
  43276f: 8b 6e 40                     	movl	0x40(%esi), %ebp
  432772: 89 0c ae                     	movl	%ecx, (%esi,%ebp,4)
  432775: 01 46 40                     	addl	%eax, 0x40(%esi)
  432778: 83 c7 04                     	addl	$0x4, %edi
  43277b: 8d 4f fe                     	leal	-0x2(%edi), %ecx
  43277e: 83 c2 10                     	addl	$0x10, %edx
  432781: 83 f9 10                     	cmpl	$0x10, %ecx
  432784: 72 9a                        	jb	0x432720 <.text+0x31720>
  432786: 5f                           	popl	%edi
  432787: 5e                           	popl	%esi
  432788: 5d                           	popl	%ebp
  432789: 5b                           	popl	%ebx
  43278a: c2 08 00                     	retl	$0x8
  43278d: cc                           	int3
  43278e: cc                           	int3
  43278f: cc                           	int3


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  428fe0: 6a ff                        	pushl	$-0x1
  428fe2: 68 f4 19 4a 00               	pushl	$0x4a19f4               # imm = 0x4A19F4
  428fe7: 64 a1 00 00 00 00            	movl	%fs:0x0, %eax
  428fed: 50                           	pushl	%eax
  428fee: 51                           	pushl	%ecx
  428fef: 53                           	pushl	%ebx
  428ff0: 56                           	pushl	%esi
  428ff1: 57                           	pushl	%edi
  428ff2: a1 48 ea 4b 00               	movl	0x4bea48, %eax
  428ff7: 33 c4                        	xorl	%esp, %eax
  428ff9: 50                           	pushl	%eax
  428ffa: 8d 44 24 14                  	leal	0x14(%esp), %eax
  428ffe: 64 a3 00 00 00 00            	movl	%eax, %fs:0x0
  429004: 8b f1                        	movl	%ecx, %esi
  429006: 89 74 24 10                  	movl	%esi, 0x10(%esp)
  42900a: 8d 4e 18                     	leal	0x18(%esi), %ecx
  42900d: e8 ae fd ff ff               	calll	0x428dc0 <.text+0x27dc0>
  429012: 33 db                        	xorl	%ebx, %ebx
  429014: c7 86 5c 01 08 00 0f 00 00 00	movl	$0xf, 0x8015c(%esi)
  42901e: 89 9e 58 01 08 00            	movl	%ebx, 0x80158(%esi)
  429024: 89 5c 24 1c                  	movl	%ebx, 0x1c(%esp)
  429028: 88 9e 48 01 08 00            	movb	%bl, 0x80148(%esi)
  42902e: 68 f0 85 42 00               	pushl	$0x4285f0               # imm = 0x4285F0
  429033: 68 d0 85 42 00               	pushl	$0x4285d0               # imm = 0x4285D0
  429038: 68 e8 03 00 00               	pushl	$0x3e8                  # imm = 0x3E8
  42903d: 6a 24                        	pushl	$0x24
  42903f: 8d be 60 01 08 00            	leal	0x80160(%esi), %edi
  429045: 57                           	pushl	%edi
  429046: c6 44 24 30 01               	movb	$0x1, 0x30(%esp)
  42904b: e8 da b7 03 00               	calll	0x46482a <.text+0x6382a>
  429050: 68 80 00 00 00               	pushl	$0x80
  429055: 8d 46 70                     	leal	0x70(%esi), %eax
  429058: 53                           	pushl	%ebx
  429059: 50                           	pushl	%eax
  42905a: c6 44 24 28 02               	movb	$0x2, 0x28(%esp)
  42905f: 89 1e                        	movl	%ebx, (%esi)
  429061: 89 5e 04                     	movl	%ebx, 0x4(%esi)
  429064: 89 5e 08                     	movl	%ebx, 0x8(%esi)
  429067: 89 5e 0c                     	movl	%ebx, 0xc(%esi)
  42906a: 89 5e 10                     	movl	%ebx, 0x10(%esi)
  42906d: 89 5e 14                     	movl	%ebx, 0x14(%esi)
  429070: 89 5e 54                     	movl	%ebx, 0x54(%esi)
  429073: 89 5e 58                     	movl	%ebx, 0x58(%esi)
  429076: 88 5e 5c                     	movb	%bl, 0x5c(%esi)
  429079: 89 5e 60                     	movl	%ebx, 0x60(%esi)
  42907c: 89 5e 64                     	movl	%ebx, 0x64(%esi)
  42907f: 88 5e 68                     	movb	%bl, 0x68(%esi)
  429082: 89 5e 6c                     	movl	%ebx, 0x6c(%esi)
  429085: e8 86 b2 03 00               	calll	0x464310 <.text+0x63310>
  42908a: 68 00 00 04 00               	pushl	$0x40000                # imm = 0x40000
  42908f: 8d 8e 04 01 00 00            	leal	0x104(%esi), %ecx
  429095: 53                           	pushl	%ebx
  429096: 51                           	pushl	%ecx
  429097: 89 9e f0 00 00 00            	movl	%ebx, 0xf0(%esi)
  42909d: 89 9e f4 00 00 00            	movl	%ebx, 0xf4(%esi)
  4290a3: 88 9e f8 00 00 00            	movb	%bl, 0xf8(%esi)
  4290a9: 89 9e fc 00 00 00            	movl	%ebx, 0xfc(%esi)
  4290af: 89 9e 00 01 00 00            	movl	%ebx, 0x100(%esi)
  4290b5: e8 56 b2 03 00               	calll	0x464310 <.text+0x63310>
  4290ba: 68 00 00 04 00               	pushl	$0x40000                # imm = 0x40000
  4290bf: 8d 96 04 01 04 00            	leal	0x40104(%esi), %edx
  4290c5: 53                           	pushl	%ebx
  4290c6: 52                           	pushl	%edx
  4290c7: e8 44 b2 03 00               	calll	0x464310 <.text+0x63310>
  4290cc: 68 a0 8c 00 00               	pushl	$0x8ca0                 # imm = 0x8CA0
  4290d1: 53                           	pushl	%ebx
  4290d2: 57                           	pushl	%edi
  4290d3: 89 9e 04 01 08 00            	movl	%ebx, 0x80104(%esi)
  4290d9: 89 9e 08 01 08 00            	movl	%ebx, 0x80108(%esi)
  4290df: 89 9e 0c 01 08 00            	movl	%ebx, 0x8010c(%esi)
  4290e5: 88 9e 11 01 08 00            	movb	%bl, 0x80111(%esi)
  4290eb: 89 9e 14 01 08 00            	movl	%ebx, 0x80114(%esi)
  4290f1: 89 9e 18 01 08 00            	movl	%ebx, 0x80118(%esi)
  4290f7: 89 9e 1c 01 08 00            	movl	%ebx, 0x8011c(%esi)
  4290fd: 89 9e 20 01 08 00            	movl	%ebx, 0x80120(%esi)
  429103: 89 9e 3c 01 08 00            	movl	%ebx, 0x8013c(%esi)
  429109: 89 9e 40 01 08 00            	movl	%ebx, 0x80140(%esi)
  42910f: e8 fc b1 03 00               	calll	0x464310 <.text+0x63310>
  429114: 83 c4 30                     	addl	$0x30, %esp
  429117: 8b ce                        	movl	%esi, %ecx
  429119: 89 9e 44 92 08 00            	movl	%ebx, 0x89244(%esi)
  42911f: 89 9e 48 92 08 00            	movl	%ebx, 0x89248(%esi)
  429125: c7 86 24 01 08 00 7f 00 00 00	movl	$0x7f, 0x80124(%esi)
  42912f: 89 9e 28 01 08 00            	movl	%ebx, 0x80128(%esi)
  429135: c7 86 2c 01 08 00 ff 07 00 00	movl	$0x7ff, 0x8012c(%esi)   # imm = 0x7FF
  42913f: c7 86 50 92 08 00 02 00 00 00	movl	$0x2, 0x89250(%esi)
  429149: 88 9e 30 01 08 00            	movb	%bl, 0x80130(%esi)
  42914f: 89 9e 34 01 08 00            	movl	%ebx, 0x80134(%esi)
  429155: 88 9e 38 01 08 00            	movb	%bl, 0x80138(%esi)
  42915b: e8 20 e6 01 00               	calll	0x447780 <.text+0x46780>
  429160: 8b c6                        	movl	%esi, %eax
  429162: 8b 4c 24 14                  	movl	0x14(%esp), %ecx
  429166: 64 89 0d 00 00 00 00         	movl	%ecx, %fs:0x0
  42916d: 59                           	popl	%ecx
  42916e: 5f                           	popl	%edi
  42916f: 5e                           	popl	%esi
  429170: 5b                           	popl	%ebx
  429171: 83 c4 10                     	addl	$0x10, %esp
  429174: c3                           	retl


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  447780: 56                           	pushl	%esi
  447781: 8b f1                        	movl	%ecx, %esi
  447783: 83 be 14 01 08 00 00         	cmpl	$0x0, 0x80114(%esi)
  44778a: 75 14                        	jne	0x4477a0 <.text+0x467a0>
  44778c: 6a 00                        	pushl	$0x0
  44778e: 6a 01                        	pushl	$0x1
  447790: 6a 01                        	pushl	$0x1
  447792: 6a 00                        	pushl	$0x0
  447794: ff 15 8c 30 4a 00            	calll	*0x4a308c
  44779a: 89 86 14 01 08 00            	movl	%eax, 0x80114(%esi)
  4477a0: 8b ce                        	movl	%esi, %ecx
  4477a2: e8 69 b1 ff ff               	calll	0x442910 <.text+0x41910>
  4477a7: c7 86 1c 01 08 00 02 00 00 00	movl	$0x2, 0x8011c(%esi)
  4477b1: c7 86 20 01 08 00 ff ff ff ff	movl	$0xffffffff, 0x80120(%esi) # imm = 0xFFFFFFFF
  4477bb: c6 86 4d 92 08 00 00         	movb	$0x0, 0x8924d(%esi)
  4477c2: c6 86 10 01 08 00 00         	movb	$0x0, 0x80110(%esi)
  4477c9: 5e                           	popl	%esi
  4477ca: c3                           	retl


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  4186b0: 8b 49 08                     	movl	0x8(%ecx), %ecx
  4186b3: 33 c0                        	xorl	%eax, %eax
  4186b5: 85 c9                        	testl	%ecx, %ecx
  4186b7: 74 4b                        	je	0x418704 <.text+0x17704>
  4186b9: 8b 54 24 08                  	movl	0x8(%esp), %edx
  4186bd: 56                           	pushl	%esi
  4186be: 8b 74 24 08                  	movl	0x8(%esp), %esi
  4186c2: 83 79 08 00                  	cmpl	$0x0, 0x8(%ecx)
  4186c6: 75 34                        	jne	0x4186fc <.text+0x176fc>
  4186c8: 39 b1 a8 00 00 00            	cmpl	%esi, 0xa8(%ecx)
  4186ce: 7f 2c                        	jg	0x4186fc <.text+0x176fc>
  4186d0: 39 b1 b0 00 00 00            	cmpl	%esi, 0xb0(%ecx)
  4186d6: 7c 24                        	jl	0x4186fc <.text+0x176fc>
  4186d8: 39 91 ac 00 00 00            	cmpl	%edx, 0xac(%ecx)
  4186de: 7f 1c                        	jg	0x4186fc <.text+0x176fc>
  4186e0: 39 91 b4 00 00 00            	cmpl	%edx, 0xb4(%ecx)
  4186e6: 7c 14                        	jl	0x4186fc <.text+0x176fc>
  4186e8: 83 b9 b8 00 00 00 00         	cmpl	$0x0, 0xb8(%ecx)
  4186ef: 75 0b                        	jne	0x4186fc <.text+0x176fc>
  4186f1: 80 b9 cb 00 00 00 00         	cmpb	$0x0, 0xcb(%ecx)
  4186f8: 8b c1                        	movl	%ecx, %eax
  4186fa: 75 07                        	jne	0x418703 <.text+0x17703>
  4186fc: 8b 49 04                     	movl	0x4(%ecx), %ecx
  4186ff: 85 c9                        	testl	%ecx, %ecx
  418701: 75 bf                        	jne	0x4186c2 <.text+0x176c2>
  418703: 5e                           	popl	%esi
  418704: c2 08 00                     	retl	$0x8
  418707: cc                           	int3
  418708: cc                           	int3
  418709: cc                           	int3
  41870a: cc                           	int3
  41870b: cc                           	int3
  41870c: cc                           	int3
  41870d: cc                           	int3
  41870e: cc                           	int3
  41870f: cc                           	int3
  418710: 8b 41 08                     	movl	0x8(%ecx), %eax
  418713: 33 d2                        	xorl	%edx, %edx
  418715: 85 c0                        	testl	%eax, %eax
  418717: 74 77                        	je	0x418790 <.text+0x17790>
  418719: 53                           	pushl	%ebx
  41871a: 8b 5c 24 0c                  	movl	0xc(%esp), %ebx
  41871e: 57                           	pushl	%edi
  41871f: 8b 7c 24 0c                  	movl	0xc(%esp), %edi
  418723: 56                           	pushl	%esi
  418724: 83 78 08 02                  	cmpl	$0x2, 0x8(%eax)
  418728: 75 59                        	jne	0x418783 <.text+0x17783>
  41872a: 83 b8 b8 00 00 00 00         	cmpl	$0x0, 0xb8(%eax)
  418731: 75 50                        	jne	0x418783 <.text+0x17783>
  418733: 8b b0 98 00 00 00            	movl	0x98(%eax), %esi
  418739: 33 c9                        	xorl	%ecx, %ecx
  41873b: 38 88 9c 00 00 00            	cmpb	%cl, 0x9c(%eax)
  418741: 75 23                        	jne	0x418766 <.text+0x17766>
  418743: 85 f6                        	testl	%esi, %esi
  418745: 7e 3c                        	jle	0x418783 <.text+0x17783>
  418747: 8b 54 88 58                  	movl	0x58(%eax,%ecx,4), %edx
  41874b: 85 d2                        	testl	%edx, %edx
  41874d: 74 04                        	je	0x418753 <.text+0x17753>
  41874f: 3b d7                        	cmpl	%edi, %edx
  418751: 74 09                        	je	0x41875c <.text+0x1775c>
  418753: 83 c1 01                     	addl	$0x1, %ecx
  418756: 3b ce                        	cmpl	%esi, %ecx
  418758: 7c ed                        	jl	0x418747 <.text+0x17747>
  41875a: eb 27                        	jmp	0x418783 <.text+0x17783>
  41875c: f7 c3 00 00 00 40            	testl	$0x40000000, %ebx       # imm = 0x40000000
  418762: 74 26                        	je	0x41878a <.text+0x1778a>
  418764: eb 1d                        	jmp	0x418783 <.text+0x17783>
  418766: 85 f6                        	testl	%esi, %esi
  418768: 7e 19                        	jle	0x418783 <.text+0x17783>
  41876a: 8d 9b 00 00 00 00            	leal	(%ebx), %ebx
  418770: 8b 54 88 58                  	movl	0x58(%eax,%ecx,4), %edx
  418774: 85 d2                        	testl	%edx, %edx
  418776: 74 04                        	je	0x41877c <.text+0x1777c>
  418778: 3b d7                        	cmpl	%edi, %edx
  41877a: 74 0e                        	je	0x41878a <.text+0x1778a>
  41877c: 83 c1 01                     	addl	$0x1, %ecx
  41877f: 3b ce                        	cmpl	%esi, %ecx
  418781: 7c ed                        	jl	0x418770 <.text+0x17770>
  418783: 8b 40 04                     	movl	0x4(%eax), %eax
  418786: 85 c0                        	testl	%eax, %eax
  418788: 75 9a                        	jne	0x418724 <.text+0x17724>
  41878a: 5e                           	popl	%esi
  41878b: 5f                           	popl	%edi
  41878c: 5b                           	popl	%ebx
  41878d: c2 08 00                     	retl	$0x8
  418790: 8b c2                        	movl	%edx, %eax
  418792: c2 08 00                     	retl	$0x8


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  4188e0: 8b 49 08                     	movl	0x8(%ecx), %ecx
  4188e3: 33 c0                        	xorl	%eax, %eax
  4188e5: 85 c9                        	testl	%ecx, %ecx
  4188e7: 0f 84 25 01 00 00            	je	0x418a12 <.text+0x17a12>
  4188ed: 53                           	pushl	%ebx
  4188ee: 55                           	pushl	%ebp
  4188ef: 8b 6c 24 10                  	movl	0x10(%esp), %ebp
  4188f3: 56                           	pushl	%esi
  4188f4: 8b 74 24 10                  	movl	0x10(%esp), %esi
  4188f8: 57                           	pushl	%edi
  4188f9: 8b 7c 24 1c                  	movl	0x1c(%esp), %edi
  4188fd: bb 01 00 00 00               	movl	$0x1, %ebx
  418902: 39 59 08                     	cmpl	%ebx, 0x8(%ecx)
  418905: 0f 85 f8 00 00 00            	jne	0x418a03 <.text+0x17a03>
  41890b: 83 b9 b8 00 00 00 00         	cmpl	$0x0, 0xb8(%ecx)
  418912: 0f 85 eb 00 00 00            	jne	0x418a03 <.text+0x17a03>
  418918: 8b 91 a8 00 00 00            	movl	0xa8(%ecx), %edx
  41891e: 85 d2                        	testl	%edx, %edx
  418920: 75 18                        	jne	0x41893a <.text+0x1793a>
  418922: 39 91 b0 00 00 00            	cmpl	%edx, 0xb0(%ecx)
  418928: 75 10                        	jne	0x41893a <.text+0x1793a>
  41892a: 39 91 ac 00 00 00            	cmpl	%edx, 0xac(%ecx)
  418930: 75 08                        	jne	0x41893a <.text+0x1793a>
  418932: 39 91 b4 00 00 00            	cmpl	%edx, 0xb4(%ecx)
  418938: 74 2c                        	je	0x418966 <.text+0x17966>
  41893a: 3b d5                        	cmpl	%ebp, %edx
  41893c: 0f 8f bd 00 00 00            	jg	0x4189ff <.text+0x179ff>
  418942: 39 a9 b0 00 00 00            	cmpl	%ebp, 0xb0(%ecx)
  418948: 0f 8c b1 00 00 00            	jl	0x4189ff <.text+0x179ff>
  41894e: 39 b9 ac 00 00 00            	cmpl	%edi, 0xac(%ecx)
  418954: 0f 8f a5 00 00 00            	jg	0x4189ff <.text+0x179ff>
  41895a: 39 b9 b4 00 00 00            	cmpl	%edi, 0xb4(%ecx)
  418960: 0f 8c 99 00 00 00            	jl	0x4189ff <.text+0x179ff>
  418966: 8b 51 14                     	movl	0x14(%ecx), %edx
  418969: 83 c2 ff                     	addl	$-0x1, %edx
  41896c: 83 fa 04                     	cmpl	$0x4, %edx
  41896f: 0f 87 8a 00 00 00            	ja	0x4189ff <.text+0x179ff>
  418975: ff 24 95 18 8a 41 00         	jmpl	*0x418a18(,%edx,4)
  41897c: 80 b9 9c 00 00 00 00         	cmpb	$0x0, 0x9c(%ecx)
  418983: 75 0a                        	jne	0x41898f <.text+0x1798f>
  418985: 81 fe 01 02 00 00            	cmpl	$0x201, %esi            # imm = 0x201
  41898b: 75 72                        	jne	0x4189ff <.text+0x179ff>
  41898d: eb 6e                        	jmp	0x4189fd <.text+0x179fd>
  41898f: 38 1d 6c 72 55 00            	cmpb	%bl, 0x55726c
  418995: 75 68                        	jne	0x4189ff <.text+0x179ff>
  418997: eb 64                        	jmp	0x4189fd <.text+0x179fd>
  418999: 80 b9 9c 00 00 00 00         	cmpb	$0x0, 0x9c(%ecx)
  4189a0: 75 0a                        	jne	0x4189ac <.text+0x179ac>
  4189a2: 81 fe 04 02 00 00            	cmpl	$0x204, %esi            # imm = 0x204
  4189a8: 75 55                        	jne	0x4189ff <.text+0x179ff>
  4189aa: eb 51                        	jmp	0x4189fd <.text+0x179fd>
  4189ac: 38 1d 78 72 55 00            	cmpb	%bl, 0x557278
  4189b2: 75 4b                        	jne	0x4189ff <.text+0x179ff>
  4189b4: eb 47                        	jmp	0x4189fd <.text+0x179fd>
  4189b6: 80 b9 9c 00 00 00 00         	cmpb	$0x0, 0x9c(%ecx)
  4189bd: 75 0a                        	jne	0x4189c9 <.text+0x179c9>
  4189bf: 81 fe 07 02 00 00            	cmpl	$0x207, %esi            # imm = 0x207
  4189c5: 75 38                        	jne	0x4189ff <.text+0x179ff>
  4189c7: eb 34                        	jmp	0x4189fd <.text+0x179fd>
  4189c9: 38 1d 84 72 55 00            	cmpb	%bl, 0x557284
  4189cf: 75 2e                        	jne	0x4189ff <.text+0x179ff>
  4189d1: eb 2a                        	jmp	0x4189fd <.text+0x179fd>
  4189d3: 81 fe 0a 02 00 00            	cmpl	$0x20a, %esi            # imm = 0x20A
  4189d9: 75 24                        	jne	0x4189ff <.text+0x179ff>
  4189db: 8b 54 24 20                  	movl	0x20(%esp), %edx
  4189df: c1 ea 10                     	shrl	$0x10, %edx
  4189e2: 66 85 d2                     	testw	%dx, %dx
  4189e5: 7e 18                        	jle	0x4189ff <.text+0x179ff>
  4189e7: eb 14                        	jmp	0x4189fd <.text+0x179fd>
  4189e9: 81 fe 0a 02 00 00            	cmpl	$0x20a, %esi            # imm = 0x20A
  4189ef: 75 0e                        	jne	0x4189ff <.text+0x179ff>
  4189f1: 8b 54 24 20                  	movl	0x20(%esp), %edx
  4189f5: c1 ea 10                     	shrl	$0x10, %edx
  4189f8: 66 85 d2                     	testw	%dx, %dx
  4189fb: 7d 02                        	jge	0x4189ff <.text+0x179ff>
  4189fd: 8b c1                        	movl	%ecx, %eax
  4189ff: 85 c0                        	testl	%eax, %eax
  418a01: 75 0b                        	jne	0x418a0e <.text+0x17a0e>
  418a03: 8b 49 04                     	movl	0x4(%ecx), %ecx


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  418a60: 53                           	pushl	%ebx
  418a61: 56                           	pushl	%esi
  418a62: 8b 71 08                     	movl	0x8(%ecx), %esi
  418a65: 32 db                        	xorb	%bl, %bl
  418a67: 85 f6                        	testl	%esi, %esi
  418a69: 0f 84 84 01 00 00            	je	0x418bf3 <.text+0x17bf3>
  418a6f: 55                           	pushl	%ebp
  418a70: 8b 6c 24 14                  	movl	0x14(%esp), %ebp
  418a74: 57                           	pushl	%edi
  418a75: 83 7e 08 05                  	cmpl	$0x5, 0x8(%esi)
  418a79: 0f 85 58 01 00 00            	jne	0x418bd7 <.text+0x17bd7>
  418a7f: 8b 44 24 14                  	movl	0x14(%esp), %eax
  418a83: 39 86 a8 00 00 00            	cmpl	%eax, 0xa8(%esi)
  418a89: 0f 8f c1 00 00 00            	jg	0x418b50 <.text+0x17b50>
  418a8f: 39 86 b0 00 00 00            	cmpl	%eax, 0xb0(%esi)
  418a95: 0f 8c b5 00 00 00            	jl	0x418b50 <.text+0x17b50>
  418a9b: 39 ae ac 00 00 00            	cmpl	%ebp, 0xac(%esi)
  418aa1: 0f 8f a9 00 00 00            	jg	0x418b50 <.text+0x17b50>
  418aa7: 39 ae b4 00 00 00            	cmpl	%ebp, 0xb4(%esi)
  418aad: 0f 8c 9d 00 00 00            	jl	0x418b50 <.text+0x17b50>
  418ab3: bf 02 00 00 00               	movl	$0x2, %edi
  418ab8: 39 be c4 00 00 00            	cmpl	%edi, 0xc4(%esi)
  418abe: 0f 84 0d 01 00 00            	je	0x418bd1 <.text+0x17bd1>
  418ac4: 8b 86 d4 00 00 00            	movl	0xd4(%esi), %eax
  418aca: 85 c0                        	testl	%eax, %eax
  418acc: 74 19                        	je	0x418ae7 <.text+0x17ae7>
  418ace: 80 38 00                     	cmpb	$0x0, (%eax)
  418ad1: 74 14                        	je	0x418ae7 <.text+0x17ae7>
  418ad3: c6 00 00                     	movb	$0x0, (%eax)
  418ad6: 8b 86 d4 00 00 00            	movl	0xd4(%esi), %eax
  418adc: 50                           	pushl	%eax
  418add: e8 7e b1 ff ff               	calll	0x413c60 <.text+0x12c60>
  418ae2: 83 c4 04                     	addl	$0x4, %esp
  418ae5: b3 01                        	movb	$0x1, %bl
  418ae7: 8b 8e d8 00 00 00            	movl	0xd8(%esi), %ecx
  418aed: 85 c9                        	testl	%ecx, %ecx
  418aef: 74 24                        	je	0x418b15 <.text+0x17b15>
  418af1: 80 39 01                     	cmpb	$0x1, (%ecx)
  418af4: 74 1f                        	je	0x418b15 <.text+0x17b15>
  418af6: e8 f5 19 ff ff               	calll	0x40a4f0 <.text+0x94f0>
  418afb: 8b 8e d8 00 00 00            	movl	0xd8(%esi), %ecx
  418b01: c6 01 01                     	movb	$0x1, (%ecx)
  418b04: 8b 96 d8 00 00 00            	movl	0xd8(%esi), %edx
  418b0a: 52                           	pushl	%edx
  418b0b: e8 50 b1 ff ff               	calll	0x413c60 <.text+0x12c60>
  418b10: 83 c4 04                     	addl	$0x4, %esp
  418b13: b3 01                        	movb	$0x1, %bl
  418b15: 8b 86 dc 00 00 00            	movl	0xdc(%esi), %eax
  418b1b: 85 c0                        	testl	%eax, %eax
  418b1d: 74 19                        	je	0x418b38 <.text+0x17b38>
  418b1f: 80 38 00                     	cmpb	$0x0, (%eax)
  418b22: 74 14                        	je	0x418b38 <.text+0x17b38>
  418b24: c6 00 00                     	movb	$0x0, (%eax)
  418b27: 8b 86 dc 00 00 00            	movl	0xdc(%esi), %eax
  418b2d: 50                           	pushl	%eax
  418b2e: e8 2d b1 ff ff               	calll	0x413c60 <.text+0x12c60>
  418b33: 83 c4 04                     	addl	$0x4, %esp
  418b36: b3 01                        	movb	$0x1, %bl
  418b38: 8b 8e e0 00 00 00            	movl	0xe0(%esi), %ecx
  418b3e: 85 c9                        	testl	%ecx, %ecx
  418b40: 0f 84 8b 00 00 00            	je	0x418bd1 <.text+0x17bd1>
  418b46: e8 95 55 04 00               	calll	0x45e0e0 <.text+0x5d0e0>
  418b4b: e9 81 00 00 00               	jmp	0x418bd1 <.text+0x17bd1>
  418b50: bf 01 00 00 00               	movl	$0x1, %edi
  418b55: 39 be c4 00 00 00            	cmpl	%edi, 0xc4(%esi)
  418b5b: 74 74                        	je	0x418bd1 <.text+0x17bd1>
  418b5d: 8b 8e d4 00 00 00            	movl	0xd4(%esi), %ecx
  418b63: 85 c9                        	testl	%ecx, %ecx
  418b65: 74 24                        	je	0x418b8b <.text+0x17b8b>
  418b67: 80 39 01                     	cmpb	$0x1, (%ecx)
  418b6a: 74 1f                        	je	0x418b8b <.text+0x17b8b>
  418b6c: e8 7f 19 ff ff               	calll	0x40a4f0 <.text+0x94f0>
  418b71: 8b 8e d4 00 00 00            	movl	0xd4(%esi), %ecx
  418b77: c6 01 01                     	movb	$0x1, (%ecx)
  418b7a: 8b 96 d4 00 00 00            	movl	0xd4(%esi), %edx
  418b80: 52                           	pushl	%edx
  418b81: e8 da b0 ff ff               	calll	0x413c60 <.text+0x12c60>
  418b86: 83 c4 04                     	addl	$0x4, %esp
  418b89: b3 01                        	movb	$0x1, %bl
  418b8b: 8b 86 d8 00 00 00            	movl	0xd8(%esi), %eax
  418b91: 85 c0                        	testl	%eax, %eax
  418b93: 74 19                        	je	0x418bae <.text+0x17bae>
  418b95: 80 38 00                     	cmpb	$0x0, (%eax)
  418b98: 74 14                        	je	0x418bae <.text+0x17bae>
  418b9a: c6 00 00                     	movb	$0x0, (%eax)
  418b9d: 8b 86 d8 00 00 00            	movl	0xd8(%esi), %eax
  418ba3: 50                           	pushl	%eax
  418ba4: e8 b7 b0 ff ff               	calll	0x413c60 <.text+0x12c60>
  418ba9: 83 c4 04                     	addl	$0x4, %esp
  418bac: b3 01                        	movb	$0x1, %bl
  418bae: 8b 86 dc 00 00 00            	movl	0xdc(%esi), %eax
  418bb4: 85 c0                        	testl	%eax, %eax
  418bb6: 74 19                        	je	0x418bd1 <.text+0x17bd1>
  418bb8: 80 38 01                     	cmpb	$0x1, (%eax)
  418bbb: 74 14                        	je	0x418bd1 <.text+0x17bd1>
  418bbd: c6 00 01                     	movb	$0x1, (%eax)
  418bc0: 8b 8e dc 00 00 00            	movl	0xdc(%esi), %ecx
  418bc6: 51                           	pushl	%ecx
  418bc7: e8 94 b0 ff ff               	calll	0x413c60 <.text+0x12c60>
  418bcc: 83 c4 04                     	addl	$0x4, %esp
  418bcf: b3 01                        	movb	$0x1, %bl
  418bd1: 89 be c4 00 00 00            	movl	%edi, 0xc4(%esi)
  418bd7: 8b 76 04                     	movl	0x4(%esi), %esi
  418bda: 85 f6                        	testl	%esi, %esi
  418bdc: 0f 85 93 fe ff ff            	jne	0x418a75 <.text+0x17a75>
  418be2: 80 fb 01                     	cmpb	$0x1, %bl
  418be5: 5f                           	popl	%edi
  418be6: 5d                           	popl	%ebp
  418be7: 75 0a                        	jne	0x418bf3 <.text+0x17bf3>
  418be9: b9 a0 70 55 00               	movl	$0x5570a0, %ecx         # imm = 0x5570A0
  418bee: e8 dd d1 00 00               	calll	0x425dd0 <.text+0x24dd0>
  418bf3: 5e                           	popl	%esi


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  418420: 56                           	pushl	%esi
  418421: 8b c1                        	movl	%ecx, %eax
  418423: 57                           	pushl	%edi
  418424: 8d 78 58                     	leal	0x58(%eax), %edi
  418427: b9 11 00 00 00               	movl	$0x11, %ecx
  41842c: 8d 74 24 0c                  	leal	0xc(%esp), %esi
  418430: f3 a5                        	rep		movsl	(%esi), %es:(%edi)
  418432: 5f                           	popl	%edi
  418433: c7 40 08 02 00 00 00         	movl	$0x2, 0x8(%eax)
  41843a: c6 80 9c 00 00 00 00         	movb	$0x0, 0x9c(%eax)
  418441: 5e                           	popl	%esi
  418442: c2 44 00                     	retl	$0x44


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  418320: 56                           	pushl	%esi
  418321: 6a 40                        	pushl	$0x40
  418323: 8b f1                        	movl	%ecx, %esi
  418325: 6a 00                        	pushl	$0x0
  418327: 56                           	pushl	%esi
  418328: e8 e3 bf 04 00               	calll	0x464310 <.text+0x63310>
  41832d: 8b 44 24 14                  	movl	0x14(%esp), %eax
  418331: c7 46 40 00 00 00 00         	movl	$0x0, 0x40(%esi)
  418338: 89 06                        	movl	%eax, (%esi)
  41833a: b8 01 00 00 00               	movl	$0x1, %eax
  41833f: 01 46 40                     	addl	%eax, 0x40(%esi)
  418342: 83 c4 0c                     	addl	$0xc, %esp
  418345: 5e                           	popl	%esi
  418346: c2 04 00                     	retl	$0x4


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  419ea0: 56                           	pushl	%esi
  419ea1: 6a 40                        	pushl	$0x40
  419ea3: 8b f1                        	movl	%ecx, %esi
  419ea5: 6a 00                        	pushl	$0x0
  419ea7: 56                           	pushl	%esi
  419ea8: e8 63 a4 04 00               	calll	0x464310 <.text+0x63310>
  419ead: 8b 44 24 14                  	movl	0x14(%esp), %eax
  419eb1: 8b 54 24 18                  	movl	0x18(%esp), %edx
  419eb5: c7 46 40 00 00 00 00         	movl	$0x0, 0x40(%esi)
  419ebc: 89 06                        	movl	%eax, (%esi)
  419ebe: b8 01 00 00 00               	movl	$0x1, %eax
  419ec3: 01 46 40                     	addl	%eax, 0x40(%esi)
  419ec6: 8b 4e 40                     	movl	0x40(%esi), %ecx
  419ec9: 89 14 8e                     	movl	%edx, (%esi,%ecx,4)
  419ecc: 01 46 40                     	addl	%eax, 0x40(%esi)
  419ecf: 83 c4 0c                     	addl	$0xc, %esp
  419ed2: 5e                           	popl	%esi
  419ed3: c2 08 00                     	retl	$0x8


game/game.exe:	file format coff-i386

Disassembly of section .text:

00401000 <.text>:
  4326a0: 56                           	pushl	%esi
  4326a1: 6a 40                        	pushl	$0x40
  4326a3: 8b f1                        	movl	%ecx, %esi
  4326a5: 6a 00                        	pushl	$0x0
  4326a7: 56                           	pushl	%esi
  4326a8: e8 63 1c 03 00               	calll	0x464310 <.text+0x63310>
  4326ad: 8b 44 24 14                  	movl	0x14(%esp), %eax
  4326b1: 8b 54 24 18                  	movl	0x18(%esp), %edx
  4326b5: c7 46 40 00 00 00 00         	movl	$0x0, 0x40(%esi)
  4326bc: 89 06                        	movl	%eax, (%esi)
  4326be: b8 01 00 00 00               	movl	$0x1, %eax
  4326c3: 01 46 40                     	addl	%eax, 0x40(%esi)
  4326c6: 8b 4e 40                     	movl	0x40(%esi), %ecx
  4326c9: 89 14 8e                     	movl	%edx, (%esi,%ecx,4)
  4326cc: 01 46 40                     	addl	%eax, 0x40(%esi)
  4326cf: 8b 4e 40                     	movl	0x40(%esi), %ecx
  4326d2: 8b 54 24 1c                  	movl	0x1c(%esp), %edx
  4326d6: 89 14 8e                     	movl	%edx, (%esi,%ecx,4)
  4326d9: 01 46 40                     	addl	%eax, 0x40(%esi)
  4326dc: 83 c4 0c                     	addl	$0xc, %esp
  4326df: 5e                           	popl	%esi
  4326e0: c2 0c 00                     	retl	$0xc

; Floating constants from PE section mappings:

; VA 0x4a3950 bytes=7a008bfcfa210940 little-endian-double=3.141592

; VA 0x4a6598 bytes=0000000000806640 little-endian-double=180.0

; VA 0x4a34f8 bytes=0000000000408f40 little-endian-double=1000.0

; VA 0x4a6590 bytes=9a9999999999d93f little-endian-double=0.4

; Settings globals: .data has RVA 0xbc000, virtual size 0xc3958, raw size 0x10000. The following VAs are in its zero-filled virtual tail before startup/config loaders:

; VA 0x56c314: loader zero-filled image storage

; VA 0x56c318: loader zero-filled image storage

; VA 0x56c900: loader zero-filled image storage

; VA 0x56c330: loader zero-filled image storage

; VA 0x56c968: loader zero-filled image storage

; VA 0x56c8ea: loader zero-filled image storage

; VA 0x56c8e8: loader zero-filled image storage

; VA 0x56c8e9: loader zero-filled image storage

; VA 0x56c89b: loader zero-filled image storage

; VA 0x56ca11: loader zero-filled image storage

; VA 0x56c8e4: loader zero-filled image storage

; VA 0x56c908: loader zero-filled image storage

; VA 0x56c31c: loader zero-filled image storage

; VA 0x56c31d: loader zero-filled image storage

; VA 0x56c356: loader zero-filled image storage

; VA 0x56c912: loader zero-filled image storage

; VA 0x56c910: loader zero-filled image storage

; VA 0x5587d0: loader zero-filled image storage

; VA 0x5587d1: loader zero-filled image storage

; VA 0x56c440: loader zero-filled image storage

; VA 0x56c32c: loader zero-filled image storage

; Native text fullwidth-space VA 0x4bd4d8 bytes=814000

; Native text fullwidth-space VA 0x4a68b4 bytes=814000

; NumberToString format VA 0x4a667c bytes=256c6400 (%ld NUL)
