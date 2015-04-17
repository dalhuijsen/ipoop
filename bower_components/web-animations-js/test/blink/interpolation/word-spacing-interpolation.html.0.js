
assertInterpolation({
  property: 'word-spacing',
  from: '-10px',
  to: '40px'
}, [
  {at: -0.3, is: '-25px'},
  {at: 0, is: '-10px'},
  {at: 0.3, is: '5px'},
  {at: 0.6, is: '20px'},
  {at: 1, is: '40px'},
  {at: 1.5, is: '65px'},
]);
