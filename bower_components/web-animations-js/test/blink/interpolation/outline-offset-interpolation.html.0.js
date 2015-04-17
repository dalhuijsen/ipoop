
assertInterpolation({
  property: 'outline-offset',
  from: '-5px',
  to: '5px'
}, [
  {at: -0.3, is: '-8px'},
  {at: 0, is: '-5px'},
  {at: 0.3, is: '-2px'},
  {at: 0.6, is: '1px'},
  {at: 1, is: '5px'},
  {at: 1.5, is: '10px'},
]);
