/*
    Projeto Final de conclusão de curso, EmbarcaTECH;
    Aluno: Augusto Cesar Barros de Oliveira
    Grupo: ChipFlow
    Projeto: Criação de um Sistema Embarcado para acionamento automático
    de uma bomba d'água, quando a caixa d'água chegar em três níveis:
    1 - Quando o nível d'água estiver com 50% ou menos;
    2 - Quando o nível chegar ao nível 100%, acionando o desligamento;
    3 - Quando chegar a 25% ou menos, informa ao sistema que a condição 1, não foi atentida e aciona novamente a bomba. 
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

// constantes de comunicação I2C para o Display
const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

// Definição dos Leds
#define LED_PIN_VM 13
#define LED_PIN_AZ 12
#define LED_PIN_VD 11

// Definição dos botões
#define BTN_A 5
#define BTN_B 6

// Definição do pino do analógico
#define ANALOG_PIN 26  // ADC0 no Raspberry Pi Pico
#define THRESHOLD_ANALOG 2000  // Limite para acionar o LED amarelo

// Configuração do pino do buzzer
#define BUZZER_PIN 21
#define BUZZER_FREQUENCY 100 // Configuração da frequência do buzzer (em Hz)

// Variáveis globais para o display
uint8_t ssd[ssd1306_buffer_length]; // Declara um array seja um buffer de dados para um display, armazenando os pixels antes de enviar para tela.
struct render_area frame_area;    // Preparar área de renderização para o display (ssd1306_width pixels por ssd1306_n_pages páginas)

// Função do beep sonoeo
static void beep(uint pin, uint duration_ms);

// Função para limpar o display
void limpa_display() {
    memset(ssd, 0, sizeof(ssd));  // Zera o buffer do display
    render_on_display(ssd, &frame_area);    // Renderiza o buffer vazio no display
}

// Função de configuração do sistema
void setup() {
    stdio_init_all();   // Inicializa os tipos stdio padrão presentes ligados ao binário

    // Inicialização do i2c
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    gpio_init(LED_PIN_VM);
    gpio_set_dir(LED_PIN_VM, GPIO_OUT);
    
    gpio_init(LED_PIN_AZ);
    gpio_set_dir(LED_PIN_AZ, GPIO_OUT);

    gpio_init(LED_PIN_VD);
    gpio_set_dir(LED_PIN_VD, GPIO_OUT); // LED Amarelo    

    // Configuração do botão
    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A);  // Habilita pull-up interno

    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);  // Habilita pull-up interno

    // Inicialização do ADC
    adc_init();
    adc_gpio_init(ANALOG_PIN);

    // Processo de inicialização completo do OLED SSD1306
    ssd1306_init();

    // Configuração da área de renderização
    frame_area.start_column = 0;
    frame_area.end_column = ssd1306_width - 1;
    frame_area.start_page = 0;
    frame_area.end_page = ssd1306_n_pages - 1; 

    calculate_render_area_buffer_length(&frame_area);

    // Configurar o pino como saída de PWM
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);

    // Obter o slice do PWM associado ao pino
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);

    // Configurar o PWM com frequência desejada
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (BUZZER_FREQUENCY * 4096)); // Divisor de clock
    pwm_init(slice_num, &config, true);

    // Iniciar o PWM no nível baixo
    pwm_set_gpio_level(BUZZER_PIN, 0);

    // limpa o display assim que o sistema inicia.
    limpa_display();
}

// função imprime texto no oled
void imprime_texto(char *text[], int num_linhas) {
    limpa_display();
    int y = 0;
    for (uint i = 0; i < num_linhas; i++) {
        ssd1306_draw_string(ssd, 5, y, text[i]);
        y += 8;
    }
    // Renderiza a mensagem no display
    render_on_display(ssd, &frame_area);
}

// Função para caixa cheia
void caixa_cheia() {
    
    char *text[] = {
                "                ",
                "   Caixa cheia   ",
                " Bomba    ",
                " desligada  "
            };

    int num_linhas = sizeof(text) / sizeof(text[0]);  // Calcula o número de linhas
    imprime_texto(text, num_linhas);
    for (int i = 0; i < 5; i++) {
        gpio_put(LED_PIN_AZ, 1);
        sleep_ms(200);
        gpio_put(LED_PIN_AZ, 0);
        sleep_ms(200);
    }
}

void loop() {
    while(true) {
        // Verifica se o botão A foi pressionado
        if (gpio_get(BTN_A) == 0) {  // 0 significa que o botão foi pressionado (pull-up)            
            // Aguarda o botão ser solto (debounce simples)
            while (gpio_get(BTN_A) == 0) {
                sleep_ms(10);
            }
            // Limpa Display
            limpa_display();
            gpio_put(LED_PIN_VM, 1);
            gpio_put(LED_PIN_VD, 1);
            // Exibe a mensagem no display
            char *text[] = {
                "       50       ",
                "   Bomba Dagua   ",
                " Acionada   "
            };
            int num_linhas = sizeof(text) / sizeof(text[0]);  
            imprime_texto(text, num_linhas);
            sleep_ms(3500);
            gpio_put(LED_PIN_VM, 0);
            gpio_put(LED_PIN_VD, 0);
            caixa_cheia();

            // limpa de 500ms desliga o led e limpa a tela
            sleep_ms(3500);
            limpa_display();
            gpio_put(LED_PIN_VD, 0);
            gpio_put(LED_PIN_VM, 0);
        }
        
        if (gpio_get(BTN_B) == 0) {  // 0 significa que o botão foi pressionado (pull-up)
            // Aguarda o botão ser solto (debounce simples)
            while (gpio_get(BTN_B) == 0) {
                sleep_ms(10);
            }
            // Limpa o display
            limpa_display();
            gpio_put(LED_PIN_VM, 1);
            gpio_put(LED_PIN_VD, 0);
            gpio_put(LED_PIN_AZ, 0);

            // Exibe a mensagem no display
            char *text[] = {
              "                     ",
                "   25% da caixa   ",
                " Bomba   ",
                " nao acionada. "
                "  Acionado bomba   "
            };
            int num_linhas = sizeof(text) / sizeof(text[0]); 
            imprime_texto(text, num_linhas);            
            // limpa de 500ms desliga o led e limpa a tela               
            sleep_ms(2000);
            gpio_put(LED_PIN_VM, 0); 
            caixa_cheia();
            sleep_ms(3500);
            limpa_display();            
        }
        // Lê o valor do analógico
        adc_select_input(0);  
        uint16_t analog_value = adc_read(); 
        // Se o valor do analógico ultrapassar o limite, aciona o LED amarelo
        if (analog_value > THRESHOLD_ANALOG) {
            limpa_display();
            gpio_put(LED_PIN_VM, 1);
            gpio_put(LED_PIN_AZ, 0);
            gpio_put(LED_PIN_VD, 0);  // Liga o LED amarelo
            // Exibe a mensagem no display
            char *text[] = {
                "   25% da caixa   ",
                " Bomba  com ",
                "  Defeito  "
            };
            
            int num_linhas = sizeof(text) / sizeof(text[0]);  // Calcula o número de linhas
            beep(BUZZER_PIN, 500);
            imprime_texto(text, num_linhas); 
            for (int i = 0; i <10; i++) {
                gpio_put(LED_PIN_VM, 1);
                sleep_ms(500);
                gpio_put(LED_PIN_VM, 0);
                sleep_ms(500);
            }
            beep(BUZZER_PIN, 500);
            sleep_ms(2500);
            // limpeza da tela e desliga o led
            limpa_display();
            gpio_put(LED_PIN_VM, 0);
        }
    }
    
}

int main() {
    setup();
    while (true) {
        loop();
        sleep_ms(100);  // Evita verificação excessivamente rápida
    }

    return 0;
}

void beep(uint pin, uint duration_ms) {
    // Obter o slice do PWM associado ao pino
    uint slice_num = pwm_gpio_to_slice_num(pin);

    // Configurar o duty cycle para 50% (ativo)
    pwm_set_gpio_level(pin, 2048);

    // Temporização
    sleep_ms(duration_ms);

    // Desativar o sinal PWM (duty cycle 0)
    pwm_set_gpio_level(pin, 0);

    // Pausa entre os beeps
    sleep_ms(100); // Pausa de 100ms
}