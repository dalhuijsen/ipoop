
assertInterpolation({
  property: 'color',
  from: 'black',
  to: 'orange'
}, [
  {at: -0.3, is: 'black'},
  {at: 0, is: 'black'},
  {at: 0.3, is: 'rgb(77, 50, 0)'},
  {at: 0.6, is: 'rgb(153, 99, 0)'},
  {at: 1, is: 'orange'},
  {at: 1.5, is: 'rgb(255, 248, 0)'},
]);
