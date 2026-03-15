function sleep (time) {
  return new Promise((resolve) => setTimeout(resolve, time));
}

async function flex() {
    const cube_el = document.getElementById("cube");
    let pos_x = 0;
    let pos_y = 0;
    let coeff = 1;    

    const main_el      = document.getElementById("main");
    const screen_width = main_el.offsetWidth;
    const indicator_el = document.getElementById("indicator");
    
    indicator_el.textContent = "width: " +  screen_width + "px";

    for (;;) {
        if (pos_x >= screen_width || pos_x <= -100) {
            coeff *= -1;
        }

        pos_x += 6 * coeff;
        pos_y = (Math.sin(pos_x * 0.01) * 180 / Math.PI) * 2;

        cube_el.style.left = pos_x + "px";
        cube_el.style.top  = pos_y + "px";

        await sleep(25);
    }
}

flex();
