
assertInterpolation({
  property: 'letter-spacing',
  from: '-10px',
  to: '10px'
}, [
  {at: -0.3, is: '-16px'},
  {at: 0, is: '-10px'},
  {at: 0.3, is: '-4px'},
  {at: 0.6, is: '2px'},
  {at: 1, is: '10px'},
  {at: 1.5, is: '20px'},
]);
