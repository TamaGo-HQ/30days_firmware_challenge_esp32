## Research

https://www.embedded.com/implementing-finite-state-machines-in-embedded-systems/?utm_source=chatgpt.com

There are two common approaches for implementing an event based state machine like the one above:

1. Using conditional statements
2. Using lookup table

### Using conditional statements

This is a very simple approach. A switch-case statement or if-else statement is used to check each state. Within each state, another conditional statement is added to check the event triggered. A handler is then added for that state/event combination. Alternatively, we could swap the order and check the event first and then the state. If there is a state change involved, the handler returns the new state.

![image.png](attachment:53c0a8f4-9cb8-4a6e-a115-721d264cc42e:image.png)

**Pros**

- Simple implementation
- Intuitive method

**Cons**

- As the number of states and event grows, code gets messy and debugging gets difficult
- Conditional statements introduce overhead

### Table based approach

In this approach, the state machine is coded in a table with one dimension as states and the other as events. Each element in the table has the handler for the state/event combination. The table could be implemented in C using a two-dimensional array of function pointers.

![image.png](attachment:c349ad8b-c546-49c8-803b-6933de2a4b7f:image.png)

**Pros**

- Software Maintenance is more under control
- When there are many invalid state/event combinations, this leads to wastage of memory
- There is a memory penalty as the number of states and events grow

Let’s move on to exercices.

---

### Exercice 1 : Basic switch-case FSM

We’ll do this with:

- 1 LED GPIO
- 1 Button GPIO
- Polling task (simpler + deterministic for learning)

**Hardware**

```c
// GPIO definitions
#define LED_GPIO     GPIO_NUM_4      // External LED
#define BUTTON_GPIO  GPIO_NUM_12     // External button
```

Button is **active-low** (pressed → 0).

**State & Event defintions**

```c
// FSM types
typedef enum {
    STATE_INIT,
    STATE_LED_OFF,
    STATE_LED_ON
} app_state_t;

typedef enum {
    EVENT_INIT_DONE,
    EVENT_BUTTON_PRESSED
} app_event_t;

```

**FSM Core**

```c
app_state_t fsm_handle_event(app_state_t current_state, app_event_t event)
{
    switch (current_state) {

        case STATE_INIT:
            if (event == EVENT_INIT_DONE) {
                gpio_set_level(LED_GPIO, 0);   // entry action
                return STATE_LED_OFF;
            }
            break;

        case STATE_LED_OFF:
            if (event == EVENT_BUTTON_PRESSED) {
                gpio_set_level(LED_GPIO, 1);   // entry action
                return STATE_LED_ON;
            }
            break;

        case STATE_LED_ON:
            if (event == EVENT_BUTTON_PRESSED) {
                gpio_set_level(LED_GPIO, 0);   // entry action
                return STATE_LED_OFF;
            }
            break;

        default:
            return STATE_INIT;
    }

    // If no transition happened, return the same state
    return current_state;
}
```

We’ll use a queue and configure a hardware interrupt to handle button presses

```c
// ===== GPIO initialization =====
static void gpio_init(void)
{
    // LED GPIO
    gpio_config_t led_cfg = {
        .pin_bit_mask = 1ULL << LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&led_cfg);

    // Button GPIO (active-low, internal pull-up, falling edge interrupt)
    gpio_config_t btn_cfg = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&btn_cfg);

    // Install ISR service and attach handler
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL);
}
```

```c
// ===== Button ISR =====
static void IRAM_ATTR button_isr_handler(void* arg)
{
    app_event_t evt = EVENT_BUTTON_PRESSED;
    xQueueSendFromISR(event_queue, &evt, NULL);
}
```

We’ll create an fsm_task that receives the queue event and update our state machine

```c
void fsm_task(void *arg)
{
    app_state_t state = STATE_INIT;

    // Initial FSM transition
    state = fsm_handle_event(state, EVENT_INIT_DONE);

    app_event_t event;

    while (1) {
        if (xQueueReceive(event_queue, &event, portMAX_DELAY)) {
            state = fsm_handle_event(state, event);
        }
    }
}
```

Build, flash and monitor. Now with every button the LED will toggle. The satifactory feeling you get when seeing your system behaving exaclty like the state machine map you drawn is my favorite thing about state machines.

However, occasionally, I would press the button and the led toggles two consicutive times. So it responds to one button press like two button presses. This is due to “Button Bounce”.

Let’s implement this system through a Table-drive FSM now

### Exercice 2 : Table driven FSM

We start by changing the enums to include the state and event number. We’ll use it later to declare our table

```c
// FSM states and events
typedef enum {
    STATE_INIT,
    STATE_LED_OFF,
    STATE_LED_ON,
    NUM_STATES
} app_state_t;

typedef enum {
    EVENT_INIT_DONE,
    EVENT_BUTTON_PRESS,
    NUM_EVENTS
} app_event_t;
```

We need to declare our handler functions. Use your state machine diagram to know what functions you need.

```c
// ===== Handler functions =====
app_state_t init_to_off(void) {
    gpio_set_level(LED_GPIO, 0);
    return STATE_LED_OFF;
}

app_state_t off_to_on(void) {
    gpio_set_level(LED_GPIO, 1);
    return STATE_LED_ON;
}

app_state_t on_to_off(void) {
    gpio_set_level(LED_GPIO, 0);
    return STATE_LED_OFF;
}

app_state_t stay(void) {
    return current_state;
}
```

We now declare our table

```c
// ===== State/Event table =====
typedef app_state_t (*state_handler_t)(void);

state_handler_t fsm_table[NUM_STATES][NUM_EVENTS] = {
    [STATE_INIT] = {
        [EVENT_INIT_DONE] = init_to_off,
        [EVENT_BUTTON_PRESS] = stay
    },
    [STATE_LED_OFF] = {
        [EVENT_BUTTON_PRESS] = off_to_on,
        [EVENT_INIT_DONE] = stay
    },
    [STATE_LED_ON] = {
        [EVENT_BUTTON_PRESS] = on_to_off,
        [EVENT_INIT_DONE] = stay
    }
};

```

If you never saw or used something like this line

```c
typedef app_state_t (*state_handler_t)(void);
```

This is a typedef for a function pointer.

Meaning, `state_handler_t` is now a **type you can use to declare a pointer to any function** that takes no argument (void) and returns an app_state_t.

Example:

```c
// A function that matches the typedef
app_state_t off_to_on(void) {
    gpio_set_level(LED_GPIO, 1);
    return STATE_LED_ON;
}

// A variable of type state_handler_t
state_handler_t handler;

// Assign the function to the pointer
handler = off_to_on;

// Call the function through the pointer
app_state_t new_state = handler();

```

This pattern is used when we have a number of “slave” functions, and at runtime, a “master” function can choose a different one depending on its need.

But all of these slave function need to have the same signature, meaning they take the same type as input and return the same type as output.

Back to our system, the “master” function is the following fsm_dispatch function

```c
// ===== FSM dispatcher (functional) =====
static app_state_t fsm_dispatch(app_state_t current_state, app_event_t event) {
    state_handler_t handler = fsm_table[current_state][event];
    if (handler) {
        return handler(current_state);
    }
    return current_state;
}

```

As we don’t know which “slave” function it would call, we use a function pointer called handler, and we tell it it is of type “state_handler_t” meaning it would take void as input and return an app_state_t.

And that’s pretty much it, you can find the whole code under “table_fsm.c” under the main folder. Don’t forget to change your CMakeLists.txt to this before building. 

```c
idf_component_register(SRCS "table_fsm.c"
                    INCLUDE_DIRS ".")
```

---

## Next Steps

- [ ]  exercice 3 : event-driven FSM with FreeRTOS (maybe traffic light?)
- [ ]  check out the state machin component in espidf