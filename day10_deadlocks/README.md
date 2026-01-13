[FreeRTOS Deadlock](https://www.digikey.com/en/maker/projects/introduction-to-rtos-solution-to-part-10-deadlock-and-starvation/872c6a057901432e84594d79fcb2cc5d)

We’ll implement and experiment with the Dining Philosopher problem to understand more about deadlocks in concurrent systems.

this is a modified version of the code found in this repo. I just changed some syntax and included libraries to suit espidf.

 

```c
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdio.h>

enum { NUM_TASKS = 5 };          // Number of tasks (philosophers)
enum { TASK_STACK_SIZE = 2048 }; // Bytes in ESP32, words in vanilla FreeRTOS

// Globals
static SemaphoreHandle_t bin_sem;  // Wait for parameters to be read
static SemaphoreHandle_t done_sem; // Notifies main task when done
static SemaphoreHandle_t chopstick[NUM_TASKS];

// The only task: eating
void eat(void *parameters) {

  int num;
  char buf[50];

  // Copy parameter and increment semaphore count
  num = *(int *)parameters;
  xSemaphoreGive(bin_sem);

  // Take left chopstick
  xSemaphoreTake(chopstick[num], portMAX_DELAY);
  sprintf(buf, "Philosopher %i took chopstick %i\n", num, num);
  printf(buf);

  // Add some delay to force deadlock
  vTaskDelay(1);

  // Take right chopstick
  xSemaphoreTake(chopstick[(num + 1) % NUM_TASKS], portMAX_DELAY);
  sprintf(buf, "Philosopher %i took chopstick %i\n", num,
          (num + 1) % NUM_TASKS);
  printf(buf);

  // Do some eating
  sprintf(buf, "Philosopher %i is eating\n", num);
  printf(buf);
  vTaskDelay(10);

  // Put down right chopstick
  xSemaphoreGive(chopstick[(num + 1) % NUM_TASKS]);
  sprintf(buf, "Philosopher %i returned chopstick %i\n", num,
          (num + 1) % NUM_TASKS);
  printf(buf);

  // Put down left chopstick
  xSemaphoreGive(chopstick[num]);
  sprintf(buf, "Philosopher %i returned chopstick %i\n", num, num);
  printf(buf);

  // Notify main task and delete self
  xSemaphoreGive(done_sem);
  vTaskDelete(NULL);
}

void app_main(void) {
  char task_name[32];

  printf("---FreeRTOS Dining Philosophers Challenge---\n");

  // Create kernel objects before starting tasks
  bin_sem = xSemaphoreCreateBinary();
  done_sem = xSemaphoreCreateCounting(NUM_TASKS, 0);
  for (int i = 0; i < NUM_TASKS; i++) {
    chopstick[i] = xSemaphoreCreateMutex();
  }

  // Have the philosphers start eating
  for (int i = 0; i < NUM_TASKS; i++) {
    sprintf(task_name, "Philosopher %i\n", i);
    xTaskCreatePinnedToCore(eat, task_name, TASK_STACK_SIZE, (void *)&i, 1,
                            NULL, 0);
    xSemaphoreTake(bin_sem, portMAX_DELAY);
  }

  // Wait until all the philosophers are done
  for (int i = 0; i < NUM_TASKS; i++) {
    xSemaphoreTake(done_sem, portMAX_DELAY);
  }

  printf("Done! No deadlock occurred!\n");
}

```

I’ll explain the **flow step by step**, focusing on **who blocks, who runs, and why**.

I’ll also explicitly point out where **deadlock can happen.**

---

### High-level picture (before code)

- **5 philosophers = 5 FreeRTOS tasks**
- **5 chopsticks = 5 mutexes**
- Each philosopher:
    1. Takes **left chopstick**
    2. Waits a bit
    3. Takes **right chopstick**
    4. Eats
    5. Releases both

---

What each semaphore is for:

Let’s demystify the semaphores first.

### 🟦 `chopstick[i]` — **mutex**

```c
chopstick[i] = xSemaphoreCreateMutex();
```

- Each chopstick is a **mutex**
- Only **one philosopher** can hold a chopstick
- Mutex → ownership + priority inheritance

---

### 🟨 `bin_sem` — **binary semaphore (task creation sync)**

```c
bin_sem = xSemaphoreCreateBinary();
```

**Purpose:**

➡️ Prevent passing the *same `i` variable* to all tasks.

This is **not about dining philosophers**, it’s about **safe task startup**.

We’ll come back to this — it’s subtle but critical.

---

### 🟩 `done_sem` — **counting semaphore**

```c
done_sem = xSemaphoreCreateCounting(NUM_TASKS, 0);

```

**Purpose:**

➡️ Let `app_main()` wait until **all philosophers finish**.

Each philosopher gives it **once** before deleting itself.

---

app_main() — system setup & task creation

Step 1: Create all synchronization objects

```c
bin_sem = xSemaphoreCreateBinary();
done_sem = xSemaphoreCreateCounting(NUM_TASKS, 0);

for (int i = 0; i < NUM_TASKS; i++) {
  chopstick[i] = xSemaphoreCreateMutex();
}

```

✅ All semaphores and mutexes exist **before** tasks start.

---

Step 2: Create philosophers (one by one)

```c
for (int i = 0; i < NUM_TASKS; i++) {
  xTaskCreatePinnedToCore(
      eat,
      task_name,
      TASK_STACK_SIZE,
      (void *)&i,
      1,
      NULL,
      app_cpu
  );

  xSemaphoreTake(bin_sem, portMAX_DELAY);
}

```

This loop is **extremely important**.

❗ Why `bin_sem` exists

Without this semaphore:

- All tasks receive the **same pointer** `&i` as parameter
- By the time tasks run, `i == 5`
- Every philosopher thinks it is philosopher **5** (out of bounds!)

So the flow is:

1. `app_main` creates **one task**
2. That task **copies `i`**
3. The task signals: “I copied it”
4. `app_main` continues to next iteration

---

eat() task — detailed flow

Now let’s go inside **one philosopher**.

---

🔹 Step 1: Copy parameter safely

```c
num = *(int *)parameters;
xSemaphoreGive(bin_sem);

```

📌 What happens:

- `num` becomes **stable** (local copy)
- Task signals `app_main`: “You can increment `i` now”

After this point, `num` never changes.

---

🔹 Step 2: Take left chopstick

```c
xSemaphoreTake(chopstick[num], portMAX_DELAY);

```

- If chopstick is free → philosopher gets it
- If not → philosopher **blocks**

📌 Everyone does this **first**, so all 5 can succeed.

---

🔹 Step 3: Artificial delay (VERY IMPORTANT)

```c
vTaskDelay(1 / portTICK_PERIOD_MS);

```

This line **forces concurrency**.

Without it:

- One philosopher might grab both chopsticks
- No deadlock

With it:

- All philosophers grab **left chopstick**
- Then all try to grab **right chopstick**

This creates the classic deadlock condition.

---

🔹 Step 4: Take right chopstick

```c
xSemaphoreTake(chopstick[(num + 1) % NUM_TASKS], portMAX_DELAY);

```

Here’s the deadly circle:

| Philosopher | Holds | Wants |
| --- | --- | --- |
| 0 | chopstick 0 | chopstick 1 |
| 1 | chopstick 1 | chopstick 2 |
| 2 | chopstick 2 | chopstick 3 |
| 3 | chopstick 3 | chopstick 4 |
| 4 | chopstick 4 | chopstick 0 |

➡️ **Circular wait = deadlock**

All tasks block forever.

---

🔹 Step 5: Eating & releasing (only if deadlock didn’t happen)

If a philosopher **manages** to get both:

```c
vTaskDelay(10 / portTICK_PERIOD_MS);

```

Then releases **right**, then **left**:

```c
xSemaphoreGive(chopstick[(num + 1) % NUM_TASKS]);
xSemaphoreGive(chopstick[num]);

```

---

🔹 Step 6: Notify main & self-destruct

```c
xSemaphoreGive(done_sem);
vTaskDelete(NULL);

```

Each philosopher increments `done_sem` once.

---

5️⃣ Back to app_main() — waiting for completion

```c
for (int i = 0; i < NUM_TASKS; i++) {
  xSemaphoreTake(done_sem, portMAX_DELAY);
}

```

- Blocks until **5 philosophers finish**
- If **deadlock occurs** → this loop **never finishes**

So this line:

```c
println("Done! No deadlock occurred!");

```

is **optimistic**, not guaranteed 😄

---

We  run the code, and oft we get this log:

```c
---FreeRTOS Dining Philosophers Challenge---
Philosopher 0 took chopstick 0
Philosopher 1 took chopstick 1
Philosopher 2 took chopstick 2
Philosopher 3 took chopstick 3
Philosopher 4 took chopstick 4
```

Hello deadlock.

Now let’s look for a solution for this potential problem.

First solution, is to let tasks be less selfish. In the current setup, each task takes a chopstick and refuse to put it down until it takes a second chopstick and eats.

This is due to a parameter in the xSemaphoretake function called xTickstoWait which is essentially a timeout. The timeout is currently set to portMAX_DELAY which basecally means infinetly.

We can set this parameter to a value of 5 ticks.

on top of main.c, we define

```c
TickType_t mutex_timeout = 5;
```

and change the defintion of our eat function to 

```c
void eat_timeout(void *parameters) {

  int num;
  char buf[50];

  // Copy parameter and increment semaphore count
  num = *(int *)parameters;
  xSemaphoreGive(bin_sem);
  while (1) {
    // Take left chopstick
    if (xSemaphoreTake(chopstick[num], mutex_timeout) == pdTRUE) {

      sprintf(buf, "Philosopher %i took chopstick %i\n", num, num);
      printf(buf);

      // Add some delay to force deadlock
      vTaskDelay(1);

      // Take right chopstick
      if (xSemaphoreTake(chopstick[(num + 1) % NUM_TASKS], mutex_timeout) ==
          pdTRUE) {
        sprintf(buf, "Philosopher %i took chopstick %i\n", num,
                (num + 1) % NUM_TASKS);
        printf(buf);

        // Do some eating
        sprintf(buf, "Philosopher %i is eating\n", num);
        printf(buf);
        vTaskDelay(10);

        // Put down right chopstick
        xSemaphoreGive(chopstick[(num + 1) % NUM_TASKS]);
        sprintf(buf, "Philosopher %i returned chopstick %i\n", num,
                (num + 1) % NUM_TASKS);
        printf(buf);

        // Put down left chopstick
        xSemaphoreGive(chopstick[num]);
        sprintf(buf, "Philosopher %i returned chopstick %i\n", num, num);
        printf(buf);

        // Notify main task and delete self
        xSemaphoreGive(done_sem);
        vTaskDelete(NULL);
      } else {
        sprintf(buf, "Philosopher %i timed out waiting for chopstick %i\n", num,
                (num + 1) % NUM_TASKS);
        printf(buf);
        xSemaphoreGive(chopstick[num]);
        sprintf(buf, "Philosopher %i returned chopstick %i\n", num, num);
        printf(buf);
        // let the next one pick it up
        vTaskDelay(1);
      }
    } else {
      sprintf(buf, "Philosopher %i timed out waiting for chopstick %i\n", num,
              num);
      printf(buf);
    }
  }
}

```

No more deadlock, it took 38 ticks for all the philosephers to complete eating, from the moment philosopher one picked up his first chopstick till the the last philosopher (which is 2) put down her last chopstick

Although unlikely, it is possible for all philosephers to pick all their left chopstick, timeout, put downs their left chopstick at the same time, and pick them up again. Meaning that their timouts are now synced.

This issue is called livelock. the tasks are not blocked like the case of a deadlock, but they’re still not getting any work down as they are prevented from accessing the shared resource.

---

Let’s go back to our selfish philosophers with thier portMAX_DELAY.

Firs, let’s number our chopsticks from 0 to 4 (remember we have 5 chopsticks) and let’s look at our table again. Each of our philosophers has to pick up two choptsicks. one of the chopstick has a higher number of the other.

i.e. Aristotle has chopstick 1 & 2. Conficius 2 & 3.. and so on and so forth.

The proposed is solution is based on this new rule : the first chopstick picked up by any philosopher, should be the chopstick with the lower number.

So Aristotle must pick up chopstick 1 first than 2.

here is what happens without this rule

```c
---FreeRTOS Dining Philosophers Challenge---
Philosopher 0 took chopstick 0
Philosopher 1 took chopstick 1
Philosopher 2 took chopstick 2
Philosopher 3 took chopstick 3
Philosopher 4 took chopstick 4
```

however now, the philosopher 4 won’t be able to pick up chopstick 4 as it is forced to take chopstick 0 first.

So philosopher 3 picks it up instead and starts eating. and then let both chopsticks down allowing philosopher 2 to pick up chopstick 3 and and start eating and so on untill all tasks complete their work.

Here is the implementation

```c
void eat(void *parameters) {

  int num;
  char buf[50];

  num = *(int *)parameters;
  xSemaphoreGive(bin_sem);

  int left  = num;
  int right = (num + 1) % NUM_TASKS;

  int first  = (left < right) ? left  : right;
  int second = (left < right) ? right : left;

  // Take lowest-numbered chopstick first
  xSemaphoreTake(chopstick[first], portMAX_DELAY);
  sprintf(buf, "Philosopher %i took chopstick %i\n", num, first);
  printf("%s", buf);

  vTaskDelay(1);

  // Take highest-numbered chopstick second
  xSemaphoreTake(chopstick[second], portMAX_DELAY);
  sprintf(buf, "Philosopher %i took chopstick %i\n", num, second);
  printf("%s", buf);

  // Eat
  sprintf(buf, "Philosopher %i is eating\n", num);
  printf("%s", buf);
  vTaskDelay(10);

  // Release in reverse order (good practice)
  xSemaphoreGive(chopstick[second]);
  sprintf(buf, "Philosopher %i returned chopstick %i\n", num, second);
  printf("%s", buf);

  xSemaphoreGive(chopstick[first]);
  sprintf(buf, "Philosopher %i returned chopstick %i\n", num, first);
  printf("%s", buf);

  xSemaphoreGive(done_sem);
  vTaskDelete(NULL);
}
```

```c
Philosopher 0 took chopstick 0
Philosopher 1 took chopstick 1
Philosopher 2 took chopstick 2
Philosopher 3 took chopstick 3
Philosopher 3 took chopstick 4
Philosopher 3 is eating
Philosopher 3 returned chopstick 4
Philosopher 3 returned chopstick 3
Philosopher 2 took chopstick 3
Philosopher 2 is eating
....
Philosopher 4 is eating
Philosopher 4 returned chopstick 4
Philosopher 4 returned chopstick 0
I (814) DINNER: Eating Done at @ tick 54
```

As expected, philosopher number 3 started eating first, and the last one to finish is philosopher 4. However it did take 54 ticks for the dinner to finish. The solution with a time out is faster even with a 5 ticks timeout.

Let’s try another solution

---

As we are talking about dinners and tables, we should thin about waiters. And the next possible solution is ading a waiter that orchastrates who gets to eat.

This will take the form of a mutex. 

when a philosopher wants to eat (wants to pick up his first chopstick) he has to ask the waiter (intially ==1), the waiter gives him permission ( resets to 0) and the next philosopher can only eat when it the waiter is set to 1 again (when the eating philosopher phinishes eating). now i understand this can be implemented with a binary semaphore or a mutex. so what is the difference between them and who is the best option for this solition?

Can this be implemented with both?

Yes, **both work functionally**. But in RTOS design, **“can” ≠ “should”**.

Key question: what *is* the waiter semantically?

Let’s classify the waiter:

- Is it a **resource** that is *owned*?
- Or a **permission / signal**?

The waiter:

- Does NOT represent a physical resource
- Does NOT need ownership tracking
- Does NOT need priority inheritance
- Is simply: *“you may proceed”*

👉 That makes it a **signal**, not a resource.

Let’s do a recap on binary semaphores and Mutexes

| Feature / Aspect | Binary Semaphore | Mutex |
| --- | --- | --- |
| **Semantics** | Signaling / permission | Resource protection |
|  | Anyone can `give` | Ownership enforced |
|  | Anyone can `take` | Only the owner can release |
|  | No ownership | Ownership enforced |
|  | No priority inheritance | Priority inheritance enabled |
| **Typical Uses** | Event notification | Protect shared data |
|  | Gates | Protect hardware peripherals |
|  | One-at-a-time permission | Prevent priority inversion |
|  | ISR → task signaling | — |

---

 Applying this to the waiter

❌ Mutex as waiter (conceptually wrong)

If you use a mutex:

```c
xSemaphoreTake(waiter_mutex);
...
xSemaphoreGive(waiter_mutex);

```

Problems (conceptual, not functional):

- Who *owns* the waiter?
- What resource is being protected?
- Priority inheritance is pointless here
- Misleading design intent

It *works*, but it’s abusing the abstraction.

---

Binary semaphore as waiter (correct)

```c
waiter = xSemaphoreCreateBinary();
xSemaphoreGive(waiter);  // initial permission

```

Philosopher flow:

1. `xSemaphoreTake(waiter)`
2. Pick up chopsticks
3. Eat
4. Put down chopsticks
5. `xSemaphoreGive(waiter)`

This matches **exactly** what a semaphore means:

> “Permission to proceed”
> 

---

So which is the *best* option?

Best choice: **Binary Semaphore**

Because:

- Correct abstraction
- Clear intent
- No unnecessary features
- Cleaner mental model
- Matches textbook solution

Mutex: acceptable but misleading

Used sometimes in quick demos, but **not ideal**.

---

Final recommendation (what I’d write in a README)

> The waiter is a permission gate, not a shared resource.
> 
> 
> Therefore, a **binary semaphore** is the correct synchronization primitive.
> 
> Using a mutex would work but would misrepresent the problem semantics.
> 

---

> Mutex = protect a thing
> 
> 
> **Semaphore = control when things happen**
> 

Our waiter controls *when* philosophers are allowed to act.

Implementation:

```c
//put this in app main before creating the tasks
  butler = xSemaphoreCreateBinary();
  xSemaphoreGive(butler); // Butler is initially available
//and this at the top of main.c
static SemaphoreHandle_t butler;

void eat_butler(void *parameters) {

  int num;
  char buf[50];

  // Copy parameter and increment semaphore count
  num = *(int *)parameters;
  xSemaphoreGive(bin_sem);

  // Ask the butler for permission to eat
  xSemaphoreTake(butler, portMAX_DELAY);
  sprintf(buf, "Philosopher %i got permission from the butler\n", num);
  printf("%s", buf);

  // Take left chopstick
  xSemaphoreTake(chopstick[num], portMAX_DELAY);
  sprintf(buf, "Philosopher %i took chopstick %i\n", num, num);
  printf(buf);

  // Add some delay to force deadlock
  vTaskDelay(1);

  // Take right chopstick
  xSemaphoreTake(chopstick[(num + 1) % NUM_TASKS], portMAX_DELAY);
  sprintf(buf, "Philosopher %i took chopstick %i\n", num,
          (num + 1) % NUM_TASKS);
  printf(buf);

  // Do some eating
  sprintf(buf, "Philosopher %i is eating\n", num);
  printf(buf);
  vTaskDelay(10);

  // Put down right chopstick
  xSemaphoreGive(chopstick[(num + 1) % NUM_TASKS]);
  sprintf(buf, "Philosopher %i returned chopstick %i\n", num,
          (num + 1) % NUM_TASKS);
  printf(buf);

  // Put down left chopstick
  xSemaphoreGive(chopstick[num]);
  sprintf(buf, "Philosopher %i returned chopstick %i\n", num, num);
  printf(buf);

  // Release the butler
  xSemaphoreGive(butler);
  sprintf(buf, "Philosopher %i released the butler\n", num);
  printf("%s", buf);

  // Notify main task and delete self
  xSemaphoreGive(done_sem);
  vTaskDelete(NULL);
}

```

We eliminated the deadlock, but it took 57 ticks, so this is the slowest solution yet.

---

What if the waiter allowes any philosopher to eat if just has his two chopsticks available?

we ll try this solution on the next visit to this challenge.