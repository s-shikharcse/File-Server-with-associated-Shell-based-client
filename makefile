all: shell get-one-file-sig server

shell:
	@gcc -w client-shell.c -o shell

get-one-file-sig:
	@gcc -w get-one-file-sig.c -o get-one-file-sig

server:
	@gcc -w server.c -o server

clean:
@rm shell get-one-file-sig server
