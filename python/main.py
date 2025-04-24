import sys
import glob
import serial
import pyautogui
import tkinter as tk
from tkinter import ttk
from tkinter import messagebox
from time import sleep
import time


# ser = serial.Serial('/dev/ttyACM13', 115200)

def move_mouse(axis, value):
    """Move o mouse de acordo com o eixo e valor recebidos."""
    if axis == 0:
        pyautogui.moveRel(value, 0)
    elif axis == 1:
        pyautogui.moveRel(0, value)


def controle(ser):
    """
    Loop principal que lê bytes da porta serial em loop infinito.
    Agora trata q1 como keyDown('q') e q2 como keyUp('q').
    """
    pressed = set()
    last_time = {}          # ← aqui inicializamos o dict
    RELEASE_DELAY = 0.2     # ← opcional, se quiser manter timeout para WASD
    ser.timeout = 0.05

    while True:
        b = ser.read(1)
        now = time.time()

        # Se não veio nada, ainda podemos soltar teclas WASD após RELEASE_DELAY
        if not b:
            for key in list(pressed):
                # libera WASD caso passe do delay
                if key in ('w','a','s','d','j','b') and now - last_time.get(key, 0) > RELEASE_DELAY:
                    pyautogui.keyUp(key)
                    pressed.remove(key)
                    print(f"SOLTO: {key}")
            continue

        # tenta decodificar como ASCII
        try:
            char = b.decode('ascii')
        except UnicodeDecodeError:
            char = None

        # TRATAMENTO EXCLUSIVO DE q, e, r, t, u
        if char in ('q', 'e', 'r', 't', 'u'):
            flag = ser.read(1)
            if not flag:
                continue
            try:
                code = flag.decode('ascii')
            except UnicodeDecodeError:
                continue

            # '1' = pressionado, '2' = solto
            if code == '1' and char not in pressed:
                pyautogui.keyDown(char)
                pressed.add(char)
                print(f"PRESSIONADO: {char}")
            elif code == '2' and char in pressed:
                pyautogui.keyUp(char)
                pressed.remove(char)
                print(f"SOLTO: {char}")
            continue

        # teclas WASD seguem igual (com timeout para soltar)
        if char in ('w', 'a', 's', 'd','b','j'):
            last_time[char] = now
            if char not in pressed:
                pyautogui.keyDown(char)
                pressed.add(char)
                print(f"PRESSIONADO: {char}")
            continue

        # pacotes binários de eixos para mover o mouse
        if b[0] != 0xFF:
            continue

        data = ser.read(3)
        if len(data) < 3:
            continue

        axis, value = parse_data(data)
        move_mouse(axis, value)
        print(f"MOUSE: eixo={axis}, val={value}")


def serial_ports():
    """Retorna uma lista das portas seriais disponíveis na máquina."""
    ports = []
    if sys.platform.startswith('win'):
        # Windows
        for i in range(1, 256):
            port = f'COM{i}'
            try:
                s = serial.Serial(port)
                s.close()
                ports.append(port)
            except (OSError, serial.SerialException):
                pass
    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        # Linux/Cygwin
        ports = glob.glob('/dev/tty[A-Za-z]*')
    elif sys.platform.startswith('darwin'):
        # macOS
        ports = glob.glob('/dev/tty.*')
    else:
        raise EnvironmentError('Plataforma não suportada para detecção de portas seriais.')

    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    return result


def parse_data(data):
    """Interpreta os dados recebidos do buffer (axis + valor)."""
    axis = data[0]
    value = int.from_bytes(data[1:3], byteorder='little', signed=True)
    return axis, value

def conectar_porta(port_name, root, botao_conectar, status_label, mudar_cor_circulo):
    """Abre a conexão com a porta selecionada e inicia o loop de leitura."""
    if not port_name:
        messagebox.showwarning("Aviso", "Selecione uma porta serial antes de conectar.")
        return

    try:
        ser = serial.Serial(port_name, 115200, timeout=1)
        status_label.config(text=f"Conectado em {port_name}", foreground="green")
        mudar_cor_circulo("green")
        botao_conectar.config(text="Conectado")  # Update button text to indicate connection
        root.update()

        # Inicia o loop de leitura (bloqueante).
        controle(ser)

    except KeyboardInterrupt:
        print("Encerrando via KeyboardInterrupt.")
    except Exception as e:
        messagebox.showerror("Erro de Conexão", f"Não foi possível conectar em {port_name}.\nErro: {e}")
        mudar_cor_circulo("red")
    finally:
        ser.close()
        status_label.config(text="Conexão encerrada.", foreground="red")
        mudar_cor_circulo("red")



def criar_janela():
    root = tk.Tk()
    root.title("Controle de Mouse")
    root.geometry("400x250")
    root.resizable(False, False)

    # Dark mode color settings
    dark_bg = "#2e2e2e"
    dark_fg = "#ffffff"
    accent_color = "#007acc"
    root.configure(bg=dark_bg)

    style = ttk.Style(root)
    style.theme_use("clam")
    style.configure("TFrame", background=dark_bg)
    style.configure("TLabel", background=dark_bg, foreground=dark_fg, font=("Segoe UI", 11))
    style.configure("TButton", font=("Segoe UI", 10, "bold"),
                    foreground=dark_fg, background="#444444", borderwidth=0)
    style.map("TButton", background=[("active", "#555555")])
    style.configure("Accent.TButton", font=("Segoe UI", 12, "bold"),
                    foreground=dark_fg, background=accent_color, padding=6)
    style.map("Accent.TButton", background=[("active", "#005f9e")])

    # Updated combobox styling to match the dark GUI color
    style.configure("TCombobox",
                    fieldbackground=dark_bg,
                    background=dark_bg,
                    foreground=dark_fg,
                    padding=4)
    style.map("TCombobox", fieldbackground=[("readonly", dark_bg)])

    # Main content frame (upper portion)
    frame_principal = ttk.Frame(root, padding="20")
    frame_principal.pack(expand=True, fill="both")

    titulo_label = ttk.Label(frame_principal, text="Controle de Mouse", font=("Segoe UI", 14, "bold"))
    titulo_label.pack(pady=(0, 10))

    porta_var = tk.StringVar(value="")

    botao_conectar = ttk.Button(
        frame_principal,
        text="Conectar e Iniciar Leitura",
        style="Accent.TButton",
        command=lambda: conectar_porta(porta_var.get(), root, botao_conectar, status_label, mudar_cor_circulo)
    )
    botao_conectar.pack(pady=10)

    # Create footer frame with grid layout to host status label, port dropdown, and status circle
    footer_frame = tk.Frame(root, bg=dark_bg)
    footer_frame.pack(side="bottom", fill="x", padx=10, pady=(10, 0))

    # Left: Status label
    status_label = tk.Label(footer_frame, text="Aguardando seleção de porta...", font=("Segoe UI", 11),
                            bg=dark_bg, fg=dark_fg)
    status_label.grid(row=0, column=0, sticky="w")

    # Center: Port selection dropdown
    portas_disponiveis = serial_ports()
    if portas_disponiveis:
        porta_var.set(portas_disponiveis[0])
    port_dropdown = ttk.Combobox(footer_frame, textvariable=porta_var,
                                 values=portas_disponiveis, state="readonly", width=10)
    port_dropdown.grid(row=0, column=1, padx=10)

    # Right: Status circle (canvas)
    circle_canvas = tk.Canvas(footer_frame, width=20, height=20, highlightthickness=0, bg=dark_bg)
    circle_item = circle_canvas.create_oval(2, 2, 18, 18, fill="red", outline="")
    circle_canvas.grid(row=0, column=2, sticky="e")

    footer_frame.columnconfigure(1, weight=1)

    def mudar_cor_circulo(cor):
        circle_canvas.itemconfig(circle_item, fill=cor)

    root.mainloop()





if __name__ == "__main__":
    criar_janela()
