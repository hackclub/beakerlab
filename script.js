const e = new Engine(null, 320 - 16, 320 - 16);

function leftTurn(a, b) {
  return (a[0]*b[1] - a[1]*b[0]) > 0;
}

const grid = {};
const TILE_SIZE = 32;
for (let x = 0; x < 10; x++)
  for (let y = 0; y < 10; y++)
    grid[[x, y]] = e.add({
      x: x * TILE_SIZE,
      y: y * TILE_SIZE,
      rotate: ~(Math.random() * 4) * 90,
      sprite: dirt_s,
      origin: [0.5, 0.5]
    });

let move = [0, 0];
e.add({
  update() {
    if (e.pressedKey("w")) move = [ 0, -1];
    if (e.pressedKey("s")) move = [ 0,  1];
    if (e.pressedKey("a")) move = [-1,  0];
    if (e.pressedKey("d")) move = [ 1,  0];
  }
})

function deltaToDegrees(delta) {
  if (delta[0] ==  1) return 90;
  if (delta[0] == -1) return 270;
  if (delta[1] ==  1) return 180;
  if (delta[1] == -1) return 0;
}

const body = [[4, 4], [4, 5]];
let apple;
const interval = e.add({
  secondsBetweenUpdates: 1/4,
  update() {
    let delta = [...move];
    
    /* did you just hit yourself/go offscreen */
    const head = body[0];
    const next = [head[0] + delta[0], head[1] + delta[1]];
    if (delta.some(x => x) && (!grid[next] || body.some(c => ""+c == ""+next))) {
      e.remove(interval);
      alert("you lost :P");
    }
  
    let last, lastDelta = delta;
    for (const i in body) {
      const chunk = body[i];
      if (last)
        delta = [last[0] - chunk[0], last[1] - chunk[1]];
  
      /* cleanup where we were before we move */
      grid[chunk].sprite = dirt_s;
  
      /* before we move. so the next chunk can go here */
      if (delta.some(x => x)) last = [...chunk];
          
      chunk[0] += delta[0], chunk[1] += delta[1];
      grid[chunk].rotate = deltaToDegrees(lastDelta);
  
      /* assign art appropriately */
      if (i == 0)
        grid[chunk].sprite = head_s;
      else if (i == body.length-1)
        grid[chunk].sprite = tail_s;
      else if (""+delta != ""+lastDelta) {
        grid[chunk].sprite = turn_s;
        if (!leftTurn(delta, lastDelta))
          grid[chunk].rotate -= 90;
      } else grid[chunk].sprite = body_s;
      
      lastDelta = delta;
    }
  
    if (!apple || ""+head == ""+apple) {
      if (apple) body.push(last);
      do {
        apple = [~~(Math.random() * 10), ~~(Math.random() * 10)];
      } while (body.some(c => ""+c == ""+apple));
      grid[apple].sprite = tomato_s;
    }
  },
})

e.start();
