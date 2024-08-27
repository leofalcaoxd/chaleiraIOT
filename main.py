# Importa as bibliotecas necessárias para a interface gráfica, comunicação MQTT e manipulação de imagens
import tkinter as tk
from tkinter import messagebox, scrolledtext, simpledialog
from PIL import Image, ImageTk
import paho.mqtt.client as mqtt
import os
import time
from datetime import datetime

# Configurações do servidor MQTT (broker), porta, usuário e tópico
MQTT_BROKER = "b37.mqtt.one"  # Endereço do broker MQTT
MQTT_PORT = 1883  # Porta padrão para conexão MQTT
MQTT_USER = "2aceiu7365"  # Nome de usuário para autenticação no broker MQTT
MQTT_PASSWORD = "16egknoswy"  # Senha para autenticação no broker MQTT
MQTT_TOPIC = "2aceiu7365/"  # Tópico MQTT para publicação e assinatura

# Variáveis globais para o status da chaleira e o tempo do timer
chaleira_status = False  # Status inicial da chaleira (desligada)
timer_duration = 10  # Duração padrão do timer em minutos
start_time = 0  # Tempo de início do timer

# Função chamada quando o cliente MQTT se conecta ao broker
def on_connect(client, userdata, flags, reasonCode, properties):
    # Adiciona uma mensagem ao log informando o resultado da conexão
    log_message(f"Conectado ao MQTT com código de resultado: {reasonCode}")
    if reasonCode == 0:
        # Conexão bem-sucedida
        log_message("Conexão MQTT bem-sucedida.")
        client.subscribe(MQTT_TOPIC)  # Inscreve no tópico para receber mensagens
        client.publish(MQTT_TOPIC, "STATUS")  # Solicita o status atual da chaleira
    else:
        # Falha na conexão
        log_message("Falha na conexão MQTT.")

# Função chamada quando uma mensagem é recebida do broker MQTT
def on_message(client, userdata, msg):
    global chaleira_status
    payload = msg.payload.decode()  # Decodifica a mensagem recebida do broker
    if payload == "ON":
        chaleira_status = True  # Atualiza o status da chaleira para ligada
        log_message("Chaleira ligada via MQTT.")
    elif payload == "OFF":
        chaleira_status = False  # Atualiza o status da chaleira para desligada
        log_message("Chaleira desligada via MQTT.")
    update_ui()  # Atualiza a interface gráfica com o novo status

# Função para ligar a chaleira
def ligar_chaleira():
    global chaleira_status, timer_duration, start_time
    if not chaleira_status:
        # Solicita ao usuário o tempo de funcionamento desejado
        timer_input = simpledialog.askinteger("Tempo de Funcionamento", "Quantos minutos a chaleira deve ficar ligada? (Deixe em branco para 10 minutos)", initialvalue=10)
        if timer_input is None:  # Verifica se o usuário cancelou a entrada
            timer_input = 10
        timer_input = validar_tempo(timer_input)  # Valida o tempo fornecido
        timer_duration = timer_input * 60  # Converte minutos para segundos
        client.publish(MQTT_TOPIC, f"TIMER {timer_duration}")  # Envia o comando com o tempo do timer
        client.publish(MQTT_TOPIC, "ON")  # Envia o comando para ligar a chaleira
        chaleira_status = True
        start_time = time.time()  # Registra o tempo de início
        log_message(f"Comando enviado: Ligar Chaleira por {timer_input} minutos.")
        update_ui()  # Atualiza a interface gráfica
    else:
        # Mensagem informativa se a chaleira já estiver ligada
        messagebox.showinfo("Status", "A chaleira já está ligada.")

# Função para desligar a chaleira
def desligar_chaleira():
    global chaleira_status
    if chaleira_status:
        client.publish(MQTT_TOPIC, "OFF")  # Envia o comando para desligar a chaleira
        chaleira_status = False
        log_message("Comando enviado: Desligar Chaleira.")
        update_ui()  # Atualiza a interface gráfica
    else:
        # Mensagem informativa se a chaleira já estiver desligada
        messagebox.showinfo("Status", "A chaleira já está desligada.")

# Função para atualizar a interface gráfica com base no status da chaleira
def update_ui():
    if chaleira_status:
        # Atualiza o status da chaleira para 'Ligada' e habilita/desabilita os botões
        status_label.config(text="Chaleira: Ligada", fg="green")
        ligar_button.config(state=tk.DISABLED)  # Desabilita o botão de ligar
        desligar_button.config(state=tk.NORMAL)  # Habilita o botão de desligar
        atualizar_tempo_restante()  # Atualiza o tempo restante no timer
    else:
        # Atualiza o status da chaleira para 'Desligada' e habilita/desabilita os botões
        status_label.config(text="Chaleira: Desligada", fg="red")
        ligar_button.config(state=tk.NORMAL)  # Habilita o botão de ligar
        desligar_button.config(state=tk.DISABLED)  # Desabilita o botão de desligar

# Função para validar o tempo máximo permitido para o timer
def validar_tempo(timer_input):
    MAX_TIME = 60  # Tempo máximo em minutos
    if timer_input > MAX_TIME:
        return MAX_TIME  # Retorna o tempo máximo se o tempo fornecido exceder o máximo permitido
    return timer_input

# Função para atualizar o tempo restante no timer
def atualizar_tempo_restante():
    global timer_duration, start_time
    if chaleira_status:
        time_left = timer_duration - int(time.time() - start_time)  # Calcula o tempo restante
        if time_left > 0:
            timer_label.config(text=f"Tempo Restante: {time_left // 60} min {time_left % 60} sec")  # Atualiza a label com o tempo restante
        else:
            timer_label.config(text="Tempo Restante: 0 min 0 sec")  # Atualiza a label quando o tempo acaba
            desligar_chaleira()  # Desliga a chaleira quando o tempo acaba
    root.after(1000, atualizar_tempo_restante)  # Atualiza o tempo restante a cada segundo

# Função para adicionar uma mensagem ao log
def log_message(message):
    log_box.configure(state=tk.NORMAL)  # Permite a edição do texto
    log_box.insert(tk.END, message + '\n')  # Insere a mensagem no final do log
    log_box.yview(tk.END)  # Rolagem automática para o fim do log
    log_box.configure(state=tk.DISABLED)  # Desabilita a edição do texto

# Função para atualizar a data e hora no rodapé
def update_datetime():
    now = datetime.now().strftime("%d/%m/%Y %H:%M:%S")  # Obtém a data e hora atual formatada
    datetime_label.config(text=f"Data e Hora: {now}")  # Atualiza a label com a data e hora
    root.after(1000, update_datetime)  # Atualiza a data e hora a cada segundo

# Função para centralizar a janela na tela
def center_window(window):
    window.update_idletasks()  # Atualiza as tarefas pendentes
    width = window.winfo_width()  # Obtém a largura da janela
    height = window.winfo_height()  # Obtém a altura da janela
    screen_width = window.winfo_screenwidth()  # Obtém a largura da tela
    screen_height = window.winfo_screenheight()  # Obtém a altura da tela
    x = (screen_width // 2) - (width // 2)  # Calcula a posição horizontal
    y = (screen_height // 2) - (height // 2)  # Calcula a posição vertical
    window.geometry(f'{width}x{height}+{x}+{y}')  # Define a posição da janela

# Configuração inicial da interface gráfica
root = tk.Tk()  # Cria a janela principal
root.title("Controle de Chaleira IOT MQTT - By Leonardo Falcão")  # Define o título da janela
root.geometry("800x600")  # Define o tamanho inicial da janela

# Função para carregar e redimensionar uma imagem
def load_image(image_path, size):
    if os.path.exists(image_path):  # Verifica se o arquivo de imagem existe
        try:
            img = Image.open(image_path)  # Abre a imagem
            img = img.resize(size, Image.Resampling.LANCZOS)  # Redimensiona a imagem
            return ImageTk.PhotoImage(img)  # Converte a imagem para um formato compatível com o Tkinter
        except Exception as e:
            print(f"Erro ao carregar ou redimensionar a imagem {image_path}: {e}")
    else:
        print(f"Imagem não encontrada: {image_path}")
    return None

# Carregar e redimensionar a imagem da chaleira
img_path = 'chaleira.jpg'
img_tk = load_image(img_path, (50, 50))  # Define o tamanho da imagem para 50x50 pixels

# Frame principal da interface
main_frame = tk.Frame(root)
main_frame.pack(fill=tk.BOTH, expand=True)  # Preenche o espaço disponível e expande

# Frame para os controles centralizados
control_frame = tk.Frame(main_frame)
control_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=10, pady=10)  # Alinha no topo e expande

# Labels e Botões de controle
status_label = tk.Label(control_frame, text="Chaleira: Desligada", font=("Arial", 24), fg="red")
status_label.pack(pady=20)  # Adiciona o rótulo de status ao frame com espaçamento vertical

# Mensagem de boas-vindas
welcome_message = tk.Label(control_frame, text="Seja Bem Vindo!\n\nEstá é a Chaleira IOT que pode ser acionada remotamente de qualquer lugar do mundo!\nSente-se e relaxe até que água fique quente para fazer seu Café ou Chá!",
                           font=("Arial", 14), justify=tk.CENTER)
welcome_message.pack(pady=10)  # Adiciona a mensagem de boas-vindas ao frame com espaçamento vertical

# Botão para ligar a chaleira com imagem
ligar_button = tk.Button(control_frame, text="Ligar Chaleira", font=("Arial", 18), command=ligar_chaleira, image=img_tk, compound=tk.LEFT)
ligar_button.pack(pady=10)  # Adiciona o botão de ligar ao frame com espaçamento vertical

# Botão para desligar a chaleira com imagem
desligar_button = tk.Button(control_frame, text="Desligar Chaleira", font=("Arial", 18), command=desligar_chaleira, state=tk.DISABLED, image=img_tk, compound=tk.LEFT)
desligar_button.pack(pady=10)  # Adiciona o botão de desligar ao frame com espaçamento vertical

# Label para exibir o tempo restante do timer
timer_label = tk.Label(control_frame, text="Tempo Restante: 0 min 0 sec", font=("Arial", 18))
timer_label.pack(pady=10)  # Adiciona o rótulo do tempo restante ao frame com espaçamento vertical

# Frame para o log de mensagens
log_frame = tk.Frame(main_frame)
log_frame.pack(side=tk.BOTTOM, fill=tk.BOTH, expand=True, padx=10, pady=10)  # Alinha no fundo e expande

# Label para o título do log
log_label = tk.Label(log_frame, text="Log MQTT", font=("Arial", 14))
log_label.pack(pady=5)  # Adiciona o título do log ao frame com espaçamento vertical

# Caixa de texto com rolagem para exibir o log
log_box = scrolledtext.ScrolledText(log_frame, height=20, state=tk.DISABLED, wrap=tk.WORD)
log_box.pack(fill=tk.BOTH, expand=True)  # Adiciona a caixa de texto ao frame com rolagem e expande

# Label para exibir a data e hora no rodapé
datetime_label = tk.Label(root, text="", font=("Arial", 12))
datetime_label.pack(side=tk.BOTTOM, pady=10)  # Adiciona a label de data e hora ao fundo da janela com espaçamento vertical

# Configuração do cliente MQTT
client = mqtt.Client(protocol=mqtt.MQTTv5)  # Cria um cliente MQTT com suporte a MQTT versão 5
client.username_pw_set(MQTT_USER, MQTT_PASSWORD)  # Configura o nome de usuário e senha para autenticação
client.on_connect = on_connect  # Define a função a ser chamada quando o cliente se conectar ao broker
client.on_message = on_message  # Define a função a ser chamada quando uma mensagem for recebida

client.connect(MQTT_BROKER, MQTT_PORT, 60)  # Conecta ao broker MQTT

# Inicia o loop do cliente MQTT em um thread separado para receber mensagens
client.loop_start()

# Atualiza a data e hora a cada segundo
update_datetime()

# Centraliza a janela na tela
center_window(root)

# Inicia o loop principal da interface gráfica
root.mainloop()
