

function vw(x) {
    return (x * window.innerWidth / 100);
}

function calc(x) {
    return Math.max(16 + (vw(10) - 16) * x, 0).toFixed(2) + "px";
}

assertInterpolation({
    property: 'width',
    from: '1em',
    to: '10vw'
}, [
    {at: -0.3, is: calc(-0.3)},
    {at: 0, is: calc(0)},
    {at: 0.3, is: calc(0.3)},
    {at: 0.6, is: calc(0.6)},
    {at: 1, is: calc(1)},
    {at: 1.5, is: calc(1.5)}
]);
