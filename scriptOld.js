let t = 0;
setAnimHandler((dt, buf) => {
  const sprites = new Float32Array(buf);
  sprites[0] = -18;
  sprites[1] = (-t * 50) % 200;
  sprites[2] = 1;
  sprites[3] = -100 + Math.cos(-t) * 50;
  sprites[4] = -100 + Math.sin(-t) * 50;
  sprites[5] = 2;
  t += dt;
});
