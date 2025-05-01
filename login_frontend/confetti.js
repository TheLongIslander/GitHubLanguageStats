const duration = 5000;
const end = Date.now() + duration;

(function frame() {
    confetti({
        particleCount: 7,
        angle: 60,
        spread: 70,
        origin: { x: 0 },
        colors: ['#00ff00', '#ffffff']
    });
    confetti({
        particleCount: 7,
        angle: 120,
        spread: 70,
        origin: { x: 1 },
        colors: ['#00ff00', '#ffffff']
    });

    if (Date.now() < end) {
        requestAnimationFrame(frame);
    }
})();
