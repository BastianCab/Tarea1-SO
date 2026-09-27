import os, pty, sys, time, signal

def verde(s): print(f"\033[32m{s}\033[0m")
def rojo(s): print(f"\033[31m{s}\033[0m")

class SesionMishell:
    def __init__(self, binario):
        self.binario = binario
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            os.execvp(self.binario, [self.binario])
            os._exit(127)
    def enviar(self, texto, espera=0.3):
        try: os.write(self.fd, texto.encode())
        except OSError: pass
        time.sleep(espera)
    def ctrl_c(self, espera=0.5):
        try: os.write(self.fd, b"\x03")
        except OSError: pass
        time.sleep(espera)
    def leer(self):
        try: return os.read(self.fd, 65536).decode(errors="replace")
        except OSError: return ""
    def sigue_viva(self):
        try:
            pid_reportado, _ = os.waitpid(self.pid, os.WNOHANG)
            return pid_reportado == 0
        except ChildProcessError:
            return False
    def cerrar(self):
        try: os.kill(self.pid, signal.SIGKILL)
        except ProcessLookupError: pass

binario = sys.argv[1]
print(f"-- Prueba 3: un job en background sobrevive a Ctrl+C del foreground -- ({binario})")
s = SesionMishell(binario)
time.sleep(0.3); s.leer()
s.enviar("sleep 5 &\n", espera=0.3)
print("Lanzamiento background:", repr(s.leer()))
s.enviar("sleep 20\n", espera=0.3)
s.ctrl_c()
viva = s.sigue_viva()
s.leer()
s.enviar("jobs\n", espera=0.3)
salida_jobs = s.leer()
print("jobs tras Ctrl+C:", repr(salida_jobs))
menciona = "sleep 5" in salida_jobs or "Done" in salida_jobs
ok = viva and menciona
(verde if ok else rojo)(f"[{'OK' if ok else 'FALLO'}] Shell viva: {viva}, background reportado: {menciona}")
if viva: s.enviar("exit\n")
s.cerrar()